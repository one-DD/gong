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

// Window function for FFT (using ESP-DSP now)
// Options: Hann window is built-in to the code
#define FFT_WINDOW_TYPE 1  // 1 = Hann window, 0 = No window

// ====================
// DISPLAY CONFIGURATION
// ====================

// Display refresh rate (ms)
#define DISPLAY_UPDATE_INTERVAL 50

// Spectrum analyzer settings
#define SPECTRUM_HEIGHT 100
#define SPECTRUM_Y_POSITION 120
#define SPECTRUM_BARS 64  // Number of frequency bars to display (max 128)

// SPL meter range
#define SPL_MIN_DB 30
#define SPL_MAX_DB 120

// Color thresholds for SPL meter
#define SPL_SAFE_THRESHOLD 70      // Green below this
#define SPL_CAUTION_THRESHOLD 85   // Yellow between safe and caution
#define SPL_WARNING_THRESHOLD 100  // Orange between caution and warning
                                   // Red above warning

// ====================
// MICROPHONE CONFIGURATION (M5.Mic)
// ====================

// M5 Microphone DMA buffer configuration
#define MIC_DMA_BUF_COUNT 4
#define MIC_DMA_BUF_LEN 1024

// Microphone oversampling (1, 2, 4, 8)
// Higher values = better quality but more CPU usage
#define MIC_OVER_SAMPLING 2

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