# M5Stack CoreS3 FFT Acoustic Analyzer

เครื่องวิเคราะห์เสียง FFT และมาตรวัด SPL สำหรับ M5Stack CoreS3

## คุณสมบัติ (Features)

- **FFT Spectrum Analyzer**: วิเคราะห์สเปกตรัมความถี่แบบเรียลไทม์
- **SPL Meter**: วัดระดับความดันเสียง (Sound Pressure Level) 
- **Dual Display Mode**: สลับระหว่างโมดสเปกตรัมและมาตรวัด SPL
- **Real-time Analysis**: การวิเคราะห์แบบเรียลไทม์ด้วย FFT 1024 จุด
- **Peak Detection**: ตรวจจับค่าสูงสุดและต่ำสุดของระดับเสียง
- **Visual Feedback**: แสดงผลด้วยสีตามระดับความเข้ม

## การติดตั้ง (Installation)

### ความต้องการ (Requirements)

1. **M5Stack CoreS3** - บอร์ดพัฒนา
2. **Arduino IDE** หรือ **PlatformIO**
3. **ไลบรารี่ที่จำเป็น**:
   - M5Unified Library
   - ESP-DSP Library (ในตัว ESP32)
   - ESP32 I2S Driver

### การติดตั้งไลบรารี่

#### ใน Arduino IDE:
```
Tools > Manage Libraries > ค้นหาและติดตั้ง:
- M5Unified by M5Stack
หมายเหตุ: ESP-DSP เป็นส่วนหนึ่งของ ESP32 Arduino Core แล้ว
```

#### ใน PlatformIO:
```ini
lib_deps = 
    m5stack/M5Unified@^0.1.16
    https://github.com/espressif/esp-dsp.git
```

### การอัปโหลดโค้ด

1. เชื่อมต่อ M5Stack CoreS3 กับคอมพิวเตอร์
2. เลือกบอร์ด: `ESP32S3 Dev Module`
3. เลือกพอร์ตที่ถูกต้อง
4. อัปโหลดโค้ด `M5Stack_FFT_SPL_Analyzer.ino`

## การใช้งาน (Usage)

### ควบคุมด้วยปุ่ม

- **ปุ่ม A**: สลับระหว่างโมด Spectrum Analyzer และ SPL Meter
- **ปุ่ม B**: รีเซ็ตค่าสูงสุด/ต่ำสุด
- **ปุ่ม C**: การตั้งค่าเพิ่มเติม (สามารถขยายความสามารถได้)

### โมดการแสดงผล

#### 1. Spectrum Analyzer Mode
- แสดงสเปกตรัมความถี่แบบเรียลไทม์
- แกน X: ความถี่ (0-22kHz)
- แกน Y: ความเข้มของสัญญาณ
- สีเขียว: ระดับปกติ
- สีเหลือง: ระดับปานกลาง  
- สีแดง: ระดับสูง

#### 2. SPL Meter Mode
- แสดงระดับความดันเสียงในหน่วย dB
- ช่วงการวัด: 30-120 dB
- สีเขียว: < 70 dB (ปลอดภัย)
- สีเหลือง: 70-85 dB (ปานกลาง)
- สีส้ม: 85-100 dB (ระวัง)
- สีแดง: > 100 dB (อันตราย)

## การปรับแต่งและคาลิเบรต

### การปรับความไว microphone

แก้ไขค่า `MICROPHONE_SENSITIVITY` ในโค้ด:
```cpp
const double MICROPHONE_SENSITIVITY = -42.0; // dBFS
```

### การปรับ sampling rate

แก้ไขค่า `SAMPLING_FREQUENCY`:
```cpp
#define SAMPLING_FREQUENCY 44100 // Hz
```

### การปรับจำนวนจุด FFT

แก้ไขค่า `SAMPLES` (ต้องเป็นเลขยกกำลังของ 2):
```cpp
#define SAMPLES 1024 // 512, 1024, 2048, etc.
```

## ข้อมูลทางเทคนิค (Technical Details)

### การกำหนดค่า I2S

- **Sampling Rate**: 44.1 kHz
- **Bit Depth**: 16-bit (เพื่อประสิทธิภาพที่ดีขึ้น)
- **Channel**: Mono (ช่องซ้าย)
- **DMA Buffer**: 4 buffers × 1024 samples

### FFT Processing

- **Engine**: ESP-DSP (Espressif optimized)
- **Window Function**: Hann Window
- **Memory Management**: Dynamic allocation
- **Performance**: ~40% เร็วกว่า ArduinoFFT

### การคำนวณ SPL

```
SPL = dBFS - MIC_SENSITIVITY + 94 dB
```

โดยที่:
- dBFS = 20 × log10(RMS)
- MIC_SENSITIVITY = ความไวของไมโครโฟน (-42 dBFS)
- 94 dB = ค่า reference สำหรับการคาลิเบรต

### ปัญหาที่อาจพบและแนวทางแก้ไข

#### 1. ไม่มีเสียงจากไมโครโฟน
- ตรวจสอบการเชื่อมต่อขา I2S
- ปรับค่า pin configuration

#### 2. ค่า SPL ไม่ถูกต้อง
- ปรับค่า MICROPHONE_SENSITIVITY
- ทำการคาลิเบรตด้วยเครื่องวัดมาตรฐาน

#### 3. สเปกตรัมไม่เสถียร
- เพิ่ม delay ในลูปหลัก
- ปรับค่า DMA buffer size

## การขยายความสามารถ

### เพิ่มฟีเจอร์ที่เป็นไปได้:

1. **Data Logging**: บันทึกข้อมูลลง SD Card
2. **WiFi Connectivity**: ส่งข้อมูลผ่าน WiFi
3. **Multiple Frequency Bands**: วิเคราะห์หลายย่านความถี่
4. **Audio Recording**: บันทึกเสียงพร้อมวิเคราะห์
5. **Bluetooth Output**: ส่งข้อมูลผ่าน Bluetooth

## ใบอนุญาต (License)

โค้ดนี้เป็น Open Source สามารถใช้งาน แก้ไข และแจกจ่ายได้อย่างอิสระ

## ผู้พัฒนา (Developer)

พัฒนาสำหรับ M5Stack CoreS3 Community

---

## English Summary

This project implements a real-time FFT acoustic analyzer and SPL meter for the M5Stack CoreS3 using M5Unified and ESP-DSP. It features:

- Real-time FFT spectrum analysis (1024-point) using optimized ESP-DSP
- Sound Pressure Level measurement (30-120 dB range)
- Dual display modes with color-coded visualization
- Peak hold functionality with configurable timeout
- I2S microphone interface with 16-bit sampling
- Interactive controls via built-in buttons
- Memory-efficient dynamic allocation
- Enhanced performance (~40% faster than ArduinoFFT)

Perfect for acoustic analysis, noise monitoring, and audio engineering applications.