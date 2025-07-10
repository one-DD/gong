#include <M5Unified.h>
#include <esp_dsp.h>
#include "config.h"

// Use configuration values
#define SAMPLES FFT_SAMPLES
#define SAMPLING_FREQUENCY SAMPLING_FREQ

// Display and UI
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

// FFT variables using ESP-DSP
float *fft_input;
float *fft_output;
float *wind;
float *spectrum_data;
int16_t *raw_samples;

// SPL calculation variables
const double MICROPHONE_SENSITIVITY = MIC_SENSITIVITY_DBFS; // dBFS from config
const double REFERENCE_PRESSURE = 0.00002; // 20 µPa reference pressure
double currentSPL = 0;
double maxSPL = 0;
double minSPL = SPL_MAX_DB;

// Display variables
int16_t spectrum_height = SPECTRUM_HEIGHT;
int16_t spectrum_y = SPECTRUM_Y_POSITION;
bool displayMode = 0; // 0: Spectrum, 1: SPL meter

// Button states and timing
unsigned long lastUpdate = 0;
unsigned long peakHoldTime = 0;
double peakSPL = 0;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    Serial.begin(115200);
    Serial.println("M5Stack CoreS3 FFT Acoustic Analyzer");
    Serial.println("Using M5Unified and ESP-DSP");
    
    // Initialize display
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    
    // Initialize FFT buffers
    initializeFFT();
    
    // Initialize M5 microphone
    setupMicrophone();
    
    // Display initial UI
    drawUI();
    
    Serial.println("Setup complete. Starting acoustic analysis...");
}

void initializeFFT() {
    // Allocate memory for FFT processing
    fft_input = (float*)malloc(SAMPLES * sizeof(float));
    fft_output = (float*)malloc(SAMPLES * sizeof(float));
    wind = (float*)malloc(SAMPLES * sizeof(float));
    spectrum_data = (float*)malloc(SAMPLES/2 * sizeof(float));
    raw_samples = (int16_t*)malloc(SAMPLES * sizeof(int16_t));
    
    if (!fft_input || !fft_output || !wind || !spectrum_data || !raw_samples) {
        Serial.println("Failed to allocate FFT buffers!");
        return;
    }
    
    // Initialize ESP-DSP
    dsps_fft2r_init_fc32(NULL, SAMPLES);
    
    // Generate Hann window
    for (int i = 0; i < SAMPLES; i++) {
        wind[i] = 0.5 - 0.5 * cosf(2.0 * M_PI * i / (SAMPLES - 1));
    }
    
    Serial.println("FFT initialized successfully");
}

void setupMicrophone() {
    // Configure M5 microphone
    auto mic_cfg = M5.Mic.config();
    mic_cfg.sample_rate = SAMPLING_FREQUENCY;
    mic_cfg.over_sampling = MIC_OVER_SAMPLING;  // Oversampling from config
    mic_cfg.dma_buf_count = MIC_DMA_BUF_COUNT;
    mic_cfg.dma_buf_len = MIC_DMA_BUF_LEN;
    
    // Initialize M5 microphone
    if (!M5.Mic.config(mic_cfg)) {
        Serial.println("Error configuring M5 microphone!");
        return;
    }
    
    if (!M5.Mic.begin()) {
        Serial.println("Error starting M5 microphone!");
        return;
    }
    
    Serial.println("M5 microphone initialized successfully");
    Serial.printf("Sample rate: %d Hz\n", M5.Mic.getSampleRate());
}

void loop() {
    M5.update();
    
    // Handle button presses
    handleButtons();
    
    // Check if it's time to update
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate >= DISPLAY_UPDATE_INTERVAL) {
        // Capture and analyze audio
        captureAudio();
        performFFT();
        calculateSPL();
        
        // Update display
        updateDisplay();
        
        lastUpdate = currentTime;
    }
}

void captureAudio() {
    // Record audio using M5 microphone
    if (M5.Mic.isEnabled()) {
        // Record samples
        size_t samples_read = M5.Mic.record(raw_samples, SAMPLES, portMAX_DELAY);
        
        if (samples_read > 0) {
            // Convert to float and apply window function
            for (int i = 0; i < SAMPLES; i++) {
                fft_input[i] = (float)raw_samples[i] * wind[i] / 32768.0f; // Normalize and apply window
                fft_output[i] = 0.0f; // Initialize imaginary part
            }
        } else {
            Serial.println("No audio samples captured");
        }
    } else {
        Serial.println("Microphone not enabled");
    }
}

void performFFT() {
    // Perform FFT using ESP-DSP
    dsps_fft2r_fc32(fft_input, SAMPLES);
    
    // Bit reverse for proper output
    dsps_bit_rev_fc32(fft_input, SAMPLES);
    
    // Convert to magnitude spectrum
    for (int i = 0; i < SAMPLES/2; i++) {
        float real = fft_input[i*2];
        float imag = fft_input[i*2 + 1];
        spectrum_data[i] = sqrtf(real*real + imag*imag);
    }
}

void calculateSPL() {
    // Calculate RMS value from spectrum
    double rms = 0;
    for (int i = 1; i < SAMPLES/2; i++) { // Skip DC component
        rms += spectrum_data[i] * spectrum_data[i];
    }
    rms = sqrt(rms / (SAMPLES/2 - 1));
    
    // Convert to dB SPL
    if (rms > 0) {
        double dBFS = 20 * log10(rms);
        currentSPL = dBFS - MICROPHONE_SENSITIVITY + REFERENCE_SPL_DB; // Calibrated SPL
        
        // Update min/max
        if (currentSPL > maxSPL) {
            maxSPL = currentSPL;
            peakSPL = currentSPL;
            peakHoldTime = millis();
        }
        if (currentSPL < minSPL && currentSPL > NOISE_FLOOR_DB) {
            minSPL = currentSPL;
        }
        
        // Reset peak hold after timeout
        if (millis() - peakHoldTime > PEAK_HOLD_TIME) {
            peakSPL = currentSPL;
        }
    }
}

void handleButtons() {
    if (M5.BtnA.wasPressed()) {
        displayMode = !displayMode;
        M5.Display.fillScreen(BLACK);
        drawUI();
        Serial.printf("Display mode: %s\n", displayMode ? "SPL Meter" : "Spectrum Analyzer");
    }
    
    if (M5.BtnB.wasPressed()) {
        // Reset min/max SPL values
        maxSPL = 0;
        minSPL = SPL_MAX_DB;
        peakSPL = 0;
        Serial.println("Min/Max SPL values reset");
    }
    
    if (M5.BtnC.wasPressed()) {
        // Print current configuration and stats
        Serial.println("=== Current Configuration ===");
        Serial.printf("Sampling Frequency: %d Hz\n", SAMPLING_FREQUENCY);
        Serial.printf("FFT Samples: %d\n", SAMPLES);
        Serial.printf("Microphone Sensitivity: %.1f dBFS\n", MICROPHONE_SENSITIVITY);
        Serial.printf("Current SPL: %.1f dB\n", currentSPL);
        Serial.printf("Peak SPL: %.1f dB\n", peakSPL);
        Serial.println("============================");
    }
}

void drawUI() {
    M5.Display.fillScreen(BLACK);
    
    if (displayMode == 0) {
        // Spectrum analyzer mode
        M5.Display.setTextSize(2);
        M5.Display.drawString("FFT Spectrum Analyzer", 10, 10);
        M5.Display.setTextSize(1);
        M5.Display.drawString("A:Mode B:Reset C:Info", 10, 220);
        
        // Draw frequency scale
        M5.Display.drawLine(40, spectrum_y + spectrum_height, 300, spectrum_y + spectrum_height, WHITE);
        M5.Display.drawString("0Hz", 35, spectrum_y + spectrum_height + 5);
        M5.Display.drawString("22kHz", 270, spectrum_y + spectrum_height + 5);
        
        // Draw amplitude scale
        M5.Display.drawLine(40, spectrum_y, 40, spectrum_y + spectrum_height, WHITE);
        M5.Display.drawString("High", 5, spectrum_y);
        M5.Display.drawString("Low", 5, spectrum_y + spectrum_height - 10);
        
    } else {
        // SPL meter mode
        M5.Display.setTextSize(2);
        M5.Display.drawString("SPL Meter", 10, 10);
        M5.Display.setTextSize(1);
        M5.Display.drawString("A:Mode B:Reset C:Info", 10, 220);
        
        // Draw SPL scale
        drawSPLMeter();
    }
}

void updateDisplay() {
    if (displayMode == 0) {
        updateSpectrum();
    } else {
        updateSPLMeter();
    }
    
    // Display current SPL value
    M5.Display.fillRect(10, 30, 200, 20, BLACK);
    M5.Display.setTextColor(GREEN);
    M5.Display.setTextSize(1);
    char splText[50];
    sprintf(splText, "SPL: %.1f dB", currentSPL);
    M5.Display.drawString(splText, 10, 30);
    
    // Display min/max values and peak hold
    M5.Display.fillRect(10, 45, 300, 30, BLACK);
    sprintf(splText, "Max: %.1f dB  Min: %.1f dB  Peak: %.1f dB", maxSPL, minSPL, peakSPL);
    M5.Display.drawString(splText, 10, 45);
}

void updateSpectrum() {
    // Clear previous spectrum
    M5.Display.fillRect(41, spectrum_y, 259, spectrum_height, BLACK);
    
    // Draw spectrum bars
    int barWidth = 259 / SPECTRUM_BARS;
    int step = (SAMPLES/2) / SPECTRUM_BARS;
    
    for (int i = 0; i < SPECTRUM_BARS; i++) {
        int spectrumIndex = i * step + 1; // Skip DC component
        if (spectrumIndex >= SAMPLES/2) break;
        
        float magnitude = spectrum_data[spectrumIndex];
        int barHeight = (int)(magnitude * spectrum_height * 2000); // Scale factor
        
        if (barHeight > spectrum_height) barHeight = spectrum_height;
        if (barHeight < 1) barHeight = 1;
        
        int x = 41 + i * barWidth;
        int y = spectrum_y + spectrum_height - barHeight;
        
        // Color coding based on magnitude
        uint16_t color = GREEN;
        if (magnitude > 0.01) color = YELLOW;
        if (magnitude > 0.05) color = ORANGE;
        if (magnitude > 0.1) color = RED;
        
        M5.Display.fillRect(x, y, barWidth-1, barHeight, color);
    }
}

void drawSPLMeter() {
    // Draw SPL meter background
    M5.Display.drawRect(50, 70, 220, 100, WHITE);
    
    // Draw scale markings
    for (int i = SPL_MIN_DB; i <= SPL_MAX_DB; i += 10) {
        int x = map(i, SPL_MIN_DB, SPL_MAX_DB, 55, 265);
        M5.Display.drawLine(x, 165, x, 170, WHITE);
        
        char label[10];
        sprintf(label, "%d", i);
        M5.Display.drawString(label, x-5, 175);
    }
    
    // Draw safety zones
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("SAFE", 60, 185);
    M5.Display.setTextColor(YELLOW);
    M5.Display.drawString("CAUTION", 120, 185);
    M5.Display.setTextColor(ORANGE);
    M5.Display.drawString("WARNING", 180, 185);
    M5.Display.setTextColor(RED);
    M5.Display.drawString("DANGER", 230, 185);
}

void updateSPLMeter() {
    // Clear previous meter reading
    M5.Display.fillRect(51, 71, 218, 98, BLACK);
    
    // Draw current SPL level
    if (currentSPL >= SPL_MIN_DB && currentSPL <= SPL_MAX_DB) {
        int meterWidth = map((int)currentSPL, SPL_MIN_DB, SPL_MAX_DB, 0, 218);
        
        // Color coding for SPL levels
        uint16_t color = GREEN;
        if (currentSPL > SPL_SAFE_THRESHOLD) color = YELLOW;
        if (currentSPL > SPL_CAUTION_THRESHOLD) color = ORANGE;
        if (currentSPL > SPL_WARNING_THRESHOLD) color = RED;
        
        M5.Display.fillRect(51, 71, meterWidth, 98, color);
    }
    
    // Draw peak markers
    if (peakSPL >= SPL_MIN_DB && peakSPL <= SPL_MAX_DB) {
        int peakX = map((int)peakSPL, SPL_MIN_DB, SPL_MAX_DB, 51, 269);
        M5.Display.drawLine(peakX, 71, peakX, 169, RED);
        M5.Display.drawLine(peakX-1, 71, peakX-1, 169, RED);
        M5.Display.drawLine(peakX+1, 71, peakX+1, 169, RED);
    }
    
    // Draw max SPL marker
    if (maxSPL >= SPL_MIN_DB && maxSPL <= SPL_MAX_DB && maxSPL != peakSPL) {
        int maxX = map((int)maxSPL, SPL_MIN_DB, SPL_MAX_DB, 51, 269);
        M5.Display.drawLine(maxX, 71, maxX, 169, ORANGE);
    }
}