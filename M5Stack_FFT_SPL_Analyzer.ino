#include <M5CoreS3.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <WiFi.h>
#include "config.h"

// Use configuration values
#define SAMPLES FFT_SAMPLES
#define SAMPLING_FREQUENCY SAMPLING_FREQ

// I2S Configuration for internal microphone
#define I2S_WS I2S_WS_PIN
#define I2S_SCK I2S_SCK_PIN
#define I2S_SD I2S_SD_PIN
#define I2S_PORT I2S_PORT_NUM

// Display and UI
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

// FFT variables
ArduinoFFT<double> FFT = ArduinoFFT<double>();
double vReal[SAMPLES];
double vImag[SAMPLES];
double frequencies[SAMPLES/2];
double magnitudes[SAMPLES/2];

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

// Button states
bool btnA_pressed = false;
bool btnB_pressed = false;
bool btnC_pressed = false;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    Serial.begin(115200);
    Serial.println("M5Stack CoreS3 FFT Acoustic Analyzer");
    
    // Initialize display
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    
    // Initialize I2S for microphone
    setupI2S();
    
    // Display initial UI
    drawUI();
    
    Serial.println("Setup complete. Starting acoustic analysis...");
}

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLING_FREQUENCY,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };
    
    esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("Error installing I2S driver: %d\n", result);
        return;
    }
    
    result = i2s_set_pin(I2S_PORT, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("Error setting I2S pins: %d\n", result);
        return;
    }
    
    Serial.println("I2S microphone initialized successfully");
}

void loop() {
    M5.update();
    
    // Handle button presses
    handleButtons();
    
    // Capture and analyze audio
    captureAudio();
    performFFT();
    calculateSPL();
    
    // Update display
    updateDisplay();
    
    delay(DISPLAY_UPDATE_INTERVAL); // Configurable delay for stability
}

void captureAudio() {
    size_t bytes_read = 0;
    int32_t samples_buffer[SAMPLES];
    
    // Read samples from I2S
    esp_err_t result = i2s_read(I2S_PORT, samples_buffer, SAMPLES * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    
    if (result == ESP_OK && bytes_read > 0) {
        // Convert samples to double and prepare for FFT
        for (int i = 0; i < SAMPLES; i++) {
            vReal[i] = (double)samples_buffer[i] / 2147483648.0; // Convert to normalized range
            vImag[i] = 0.0;
        }
    }
}

void performFFT() {
    // Apply window function
    FFT.windowing(vReal, SAMPLES, FFT_WINDOW_TYPE, FFT_FORWARD);
    
    // Compute FFT
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    
    // Compute magnitudes
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);
    
    // Store frequency bins and magnitudes for display
    for (int i = 0; i < SAMPLES/2; i++) {
        frequencies[i] = (i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
        magnitudes[i] = vReal[i];
    }
}

void calculateSPL() {
    // Calculate RMS value
    double rms = 0;
    for (int i = 1; i < SAMPLES/2; i++) { // Skip DC component
        rms += magnitudes[i] * magnitudes[i];
    }
    rms = sqrt(rms / (SAMPLES/2 - 1));
    
    // Convert to dB SPL
    if (rms > 0) {
        double dBFS = 20 * log10(rms);
        currentSPL = dBFS - MICROPHONE_SENSITIVITY + REFERENCE_SPL_DB; // Calibrated SPL
        
        // Update min/max
        if (currentSPL > maxSPL) maxSPL = currentSPL;
        if (currentSPL < minSPL && currentSPL > 0) minSPL = currentSPL;
    }
}

void handleButtons() {
    if (M5.BtnA.wasPressed()) {
        displayMode = !displayMode;
        M5.Display.fillScreen(BLACK);
        drawUI();
    }
    
    if (M5.BtnB.wasPressed()) {
        // Reset min/max SPL values
        maxSPL = 0;
        minSPL = SPL_MAX_DB;
    }
    
    if (M5.BtnC.wasPressed()) {
        // Toggle between different frequency ranges or analysis modes
        Serial.println("Button C pressed - Feature can be extended");
    }
}

void drawUI() {
    M5.Display.fillScreen(BLACK);
    
    if (displayMode == 0) {
        // Spectrum analyzer mode
        M5.Display.setTextSize(2);
        M5.Display.drawString("FFT Spectrum Analyzer", 10, 10);
        M5.Display.setTextSize(1);
        M5.Display.drawString("A:Mode B:Reset C:Config", 10, 220);
        
        // Draw frequency scale
        M5.Display.drawLine(40, spectrum_y + spectrum_height, 300, spectrum_y + spectrum_height, WHITE);
        M5.Display.drawString("0Hz", 35, spectrum_y + spectrum_height + 5);
        M5.Display.drawString("22kHz", 270, spectrum_y + spectrum_height + 5);
        
    } else {
        // SPL meter mode
        M5.Display.setTextSize(2);
        M5.Display.drawString("SPL Meter", 10, 10);
        M5.Display.setTextSize(1);
        M5.Display.drawString("A:Mode B:Reset C:Config", 10, 220);
        
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
    
    // Display min/max values
    M5.Display.fillRect(10, 45, 200, 30, BLACK);
    sprintf(splText, "Max: %.1f dB  Min: %.1f dB", maxSPL, minSPL);
    M5.Display.drawString(splText, 10, 45);
}

void updateSpectrum() {
    // Clear previous spectrum
    M5.Display.fillRect(40, spectrum_y, 260, spectrum_height, BLACK);
    
    // Draw spectrum bars
    int barWidth = 260 / (SAMPLES/8); // Show only lower frequencies
    
    for (int i = 1; i < SAMPLES/8; i++) {
        double magnitude = magnitudes[i];
        int barHeight = (int)(magnitude * spectrum_height * 1000); // Scale factor
        
        if (barHeight > spectrum_height) barHeight = spectrum_height;
        if (barHeight < 0) barHeight = 0;
        
        int x = 40 + i * barWidth;
        int y = spectrum_y + spectrum_height - barHeight;
        
        // Color coding based on magnitude
        uint16_t color = GREEN;
        if (magnitude > 0.1) color = YELLOW;
        if (magnitude > 0.3) color = RED;
        
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
    if (maxSPL >= SPL_MIN_DB && maxSPL <= SPL_MAX_DB) {
        int peakX = map((int)maxSPL, SPL_MIN_DB, SPL_MAX_DB, 51, 269);
        M5.Display.drawLine(peakX, 71, peakX, 169, RED);
    }
}