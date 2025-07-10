/*
 * Mic_FFT.ino - Simple FFT Microphone Analyzer
 * M5Stack CoreS3 with M5Unified and ESP-DSP
 * 
 * Simplified version focusing on basic FFT functionality
 */

#include <M5Unified.h>
#include <driver/i2s.h>
#include <esp_dsp.h>

// Basic configuration
#define SAMPLES 1024
#define SAMPLING_RATE 16000  // Lower rate for better performance
#define I2S_PORT I2S_NUM_0

// I2S pins for M5Stack CoreS3
#define I2S_WS 14
#define I2S_SCK 13
#define I2S_SD 12

// FFT variables
float *fft_input;
float *spectrum;
int16_t *raw_samples;

void setup() {
  // Initialize M5Stack
  auto cfg = M5.config();
  M5.begin(cfg);
  
  Serial.begin(115200);
  Serial.println("Simple Mic FFT Analyzer");
  
  // Setup display
  M5.Display.setRotation(1);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  M5.Display.drawString("Mic FFT", 120, 10);
  
  // Initialize FFT
  initFFT();
  
  // Initialize I2S microphone
  initMicrophone();
  
  Serial.println("Setup complete");
}

void initFFT() {
  // Allocate memory
  fft_input = (float*)malloc(SAMPLES * sizeof(float));
  spectrum = (float*)malloc(SAMPLES/2 * sizeof(float));
  raw_samples = (int16_t*)malloc(SAMPLES * sizeof(int16_t));
  
  if (!fft_input || !spectrum || !raw_samples) {
    Serial.println("FFT memory allocation failed!");
    return;
  }
  
  // Initialize ESP-DSP
  dsps_fft2r_init_fc32(NULL, SAMPLES);
  Serial.println("FFT initialized");
}

void initMicrophone() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLING_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  
  Serial.println("Microphone initialized");
}

void loop() {
  M5.update();
  
  // Capture audio
  captureAudio();
  
  // Perform FFT
  performFFT();
  
  // Display spectrum
  displaySpectrum();
  
  // Print max frequency component
  printPeakFrequency();
  
  delay(100);
}

void captureAudio() {
  size_t bytes_read;
  i2s_read(I2S_PORT, raw_samples, SAMPLES * sizeof(int16_t), &bytes_read, portMAX_DELAY);
  
  // Convert to float and normalize
  for (int i = 0; i < SAMPLES; i++) {
    fft_input[i] = (float)raw_samples[i] / 32768.0f;
  }
}

void performFFT() {
  // Perform FFT
  dsps_fft2r_fc32(fft_input, SAMPLES);
  dsps_bit_rev_fc32(fft_input, SAMPLES);
  
  // Calculate magnitude spectrum
  for (int i = 0; i < SAMPLES/2; i++) {
    float real = fft_input[i*2];
    float imag = fft_input[i*2 + 1];
    spectrum[i] = sqrtf(real*real + imag*imag);
  }
}

void displaySpectrum() {
  // Clear spectrum area
  M5.Display.fillRect(20, 50, 280, 120, BLACK);
  
  // Draw spectrum bars
  int barWidth = 280 / 64;  // Show 64 frequency bins
  
  for (int i = 1; i < 64; i++) {  // Skip DC component
    int specIndex = i * (SAMPLES/2) / 64;
    float magnitude = spectrum[specIndex];
    
    int barHeight = (int)(magnitude * 1000);
    if (barHeight > 120) barHeight = 120;
    if (barHeight < 1) barHeight = 1;
    
    int x = 20 + i * barWidth;
    int y = 170 - barHeight;
    
    // Color based on magnitude
    uint16_t color = GREEN;
    if (magnitude > 0.01) color = YELLOW;
    if (magnitude > 0.05) color = ORANGE;
    if (magnitude > 0.1) color = RED;
    
    M5.Display.fillRect(x, y, barWidth-1, barHeight, color);
  }
  
  // Draw frequency labels
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE);
  M5.Display.drawString("0Hz", 20, 175);
  M5.Display.drawString("8kHz", 270, 175);
}

void printPeakFrequency() {
  // Find peak frequency
  float maxMagnitude = 0;
  int peakIndex = 0;
  
  for (int i = 1; i < SAMPLES/2; i++) {
    if (spectrum[i] > maxMagnitude) {
      maxMagnitude = spectrum[i];
      peakIndex = i;
    }
  }
  
  // Calculate frequency
  float peakFreq = (float)peakIndex * SAMPLING_RATE / SAMPLES;
  
  // Display on screen
  M5.Display.fillRect(20, 190, 280, 25, BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(CYAN);
  
  char freqText[50];
  sprintf(freqText, "Peak: %.0f Hz  Mag: %.3f", peakFreq, maxMagnitude);
  M5.Display.drawString(freqText, 20, 190);
  
  // Print to serial every 10 loops
  static int counter = 0;
  if (++counter >= 10) {
    Serial.printf("Peak Frequency: %.2f Hz, Magnitude: %.4f\n", peakFreq, maxMagnitude);
    counter = 0;
  }
}