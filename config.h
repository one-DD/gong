#ifndef CONFIG_H
#define CONFIG_H

// ====================
// MICROPHONE CONFIGURATION
// ====================

// Microphone sensitivity in dBFS (adjust based on your microphone spec)
// Common values: 
// - INMP441: -26 dBFS
// - SPH0645: -26 dBFS  
// - M5Stack internal mic: -42 dBFS
#define MIC_SENSITIVITY_DBFS -42.0

// Reference SPL for calibration (typically 94 dB @ 1 kHz)
#define REFERENCE_SPL_DB 94.0

// Microphone bias voltage compensation (if needed)
#define MIC_BIAS_COMPENSATION 0.0

// ====================
// FFT CONFIGURATION  
// ====================

// Number of FFT samples (must be power of 2: 256, 512, 1024, 2048)
// Higher = better frequency resolution, slower processing
#define FFT_SAMPLES 1024

// Sampling frequency in Hz
// Max recommended: 44100 Hz for good performance
// Lower values: 22050, 16000, 8000 for less CPU usage
#define SAMPLING_FREQ 44100

// Window function for FFT
// Options: FFT_WIN_TYP_HAMMING, FFT_WIN_TYP_HANN, FFT_WIN_TYP_BLACKMAN
#define FFT_WINDOW_TYPE FFT_WIN_TYP_HAMMING

// ====================
// DISPLAY CONFIGURATION
// ====================

// Display refresh rate (ms)
#define DISPLAY_UPDATE_INTERVAL 50

// Spectrum analyzer settings
#define SPECTRUM_HEIGHT 100
#define SPECTRUM_Y_POSITION 120
#define SPECTRUM_BARS 64  // Number of frequency bars to display

// SPL meter range
#define SPL_MIN_DB 30
#define SPL_MAX_DB 120

// Color thresholds for SPL meter
#define SPL_SAFE_THRESHOLD 70      // Green below this
#define SPL_CAUTION_THRESHOLD 85   // Yellow between safe and caution
#define SPL_WARNING_THRESHOLD 100  // Orange between caution and warning
                                   // Red above warning

// ====================
// I2S CONFIGURATION
// ====================

// I2S pins for M5Stack CoreS3
#define I2S_WS_PIN 14
#define I2S_SCK_PIN 13  
#define I2S_SD_PIN 12
#define I2S_PORT_NUM I2S_NUM_0

// I2S DMA buffer configuration
#define I2S_DMA_BUF_COUNT 4
#define I2S_DMA_BUF_LEN 1024

// ====================
// CALIBRATION SETTINGS
// ====================

// Enable/disable automatic gain control
#define ENABLE_AGC false

// Manual gain adjustment (1.0 = no change)
#define MANUAL_GAIN 1.0

// Noise floor threshold (dB)
#define NOISE_FLOOR_DB 30

// Peak hold time for SPL meter (ms)
#define PEAK_HOLD_TIME 2000

// ====================
// ADVANCED SETTINGS
// ====================

// Enable debug output
#define DEBUG_MODE true

// Enable data logging to serial
#define ENABLE_SERIAL_LOGGING false

// Enable WiFi for remote monitoring (future feature)
#define ENABLE_WIFI false

// Enable SD card logging (future feature) 
#define ENABLE_SD_LOGGING false

// ====================
// FREQUENCY BAND ANALYSIS
// ====================

// Enable octave band analysis
#define ENABLE_OCTAVE_BANDS false

// Frequency bands for analysis (Hz)
#define BAND_63HZ_CENTER 63
#define BAND_125HZ_CENTER 125
#define BAND_250HZ_CENTER 250
#define BAND_500HZ_CENTER 500
#define BAND_1KHZ_CENTER 1000
#define BAND_2KHZ_CENTER 2000
#define BAND_4KHZ_CENTER 4000
#define BAND_8KHZ_CENTER 8000

#endif // CONFIG_H