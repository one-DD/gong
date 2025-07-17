/*
 * M5CoreS3 Microphone WAV Recording System
 * โปรแกรมบันทึกเสียงจากไมโครโฟนลงไฟล์ WAV ใน microSD card
 * 
 * ฟีเจอร์:
 * 1. บันทึกเสียง 10 วินาทีต่อไฟล์
 * 2. บันทึกอัตโนมัติทุกๆ 1 นาที
 * 3. บันทึกจนกว่า SD card จะเต็ม หรือกดปุ่ม Stop
 * 4. มีคำอธิบายโค้ดทุกบรรทัด
 */

#include <M5CoreS3.h>          // ไลบรารี่หลักสำหรับ M5CoreS3
#include <SD.h>                // ไลบรารี่สำหรับการใช้งาน SD card
#include <FS.h>                // ไลบรารี่สำหรับระบบไฟล์
#include <driver/i2s.h>        // ไลบรารี่สำหรับการใช้งาน I2S (Inter-IC Sound)

// ===============================
// การตั้งค่าพารามิเตอร์เสียง
// ===============================
#define SAMPLE_RATE 16000      // ความถี่การสุ่มตัวอย่าง 16kHz
#define BITS_PER_SAMPLE 16     // จำนวนบิตต่อตัวอย่าง 16 บิต
#define CHANNELS 1             // จำนวนช่องเสียง (โมโน)
#define RECORD_TIME 10         // เวลาบันทึกต่อไฟล์ (วินาที)
#define RECORDING_INTERVAL 60  // ช่วงเวลาระหว่างการบันทึก (วินาที)

// ===============================
// การตั้งค่า I2S
// ===============================
#define I2S_WS 7              // พิน Word Select (WS)
#define I2S_SCK 6             // พิน Serial Clock (SCK)
#define I2S_SD 5              // พิน Serial Data (SD)
#define I2S_PORT I2S_NUM_0    // พอร์ต I2S ที่ใช้

// ===============================
// ตัวแปรสำหรับการทำงาน
// ===============================
bool isRecording = false;                    // สถานะการบันทึก
bool autoRecordingEnabled = true;           // สถานะการบันทึกอัตโนมัติ
unsigned long lastRecordingTime = 0;        // เวลาที่บันทึกล่าสุด
unsigned long recordingStartTime = 0;       // เวลาเริ่มบันทึก
int fileCounter = 0;                        // ตัวนับไฟล์
File audioFile;                             // ตัวแปรไฟล์เสียง
int16_t *audioBuffer;                       // บัฟเฟอร์สำหรับเก็บข้อมูลเสียง
const int bufferSize = 1024;                // ขนาดบัฟเฟอร์
size_t bytesRead;                           // จำนวนไบต์ที่อ่านได้

// ===============================
// โครงสร้างข้อมูล WAV Header
// ===============================
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};           // RIFF identifier
    uint32_t fileSize;                              // ขนาดไฟล์
    char wave[4] = {'W', 'A', 'V', 'E'};           // WAVE identifier
    char fmt[4] = {'f', 'm', 't', ' '};            // Format chunk identifier
    uint32_t fmtSize = 16;                         // ขนาดของ format chunk
    uint16_t audioFormat = 1;                      // รูปแบบเสียง (PCM)
    uint16_t numChannels = CHANNELS;               // จำนวนช่องเสียง
    uint32_t sampleRate = SAMPLE_RATE;             // ความถี่การสุ่มตัวอย่าง
    uint32_t byteRate = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8;  // อัตราไบต์ต่อวินาที
    uint16_t blockAlign = CHANNELS * BITS_PER_SAMPLE / 8;              // การจัดเรียงบล็อก
    uint16_t bitsPerSample = BITS_PER_SAMPLE;      // จำนวนบิตต่อตัวอย่าง
    char data[4] = {'d', 'a', 't', 'a'};           // Data chunk identifier
    uint32_t dataSize;                             // ขนาดข้อมูลเสียง
};

// ===============================
// ฟังก์ชันเริ่มต้นระบบ I2S
// ===============================
void setupI2S() {
    Serial.println("กำลังตั้งค่า I2S...");        // แสดงข้อความในการตั้งค่า
    
    // การตั้งค่า I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),  // โหมดหลักและรับข้อมูล
        .sample_rate = SAMPLE_RATE,                           // ความถี่การสุ่มตัวอย่าง
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,        // จำนวนบิตต่อตัวอย่าง
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,         // รูปแบบช่องเสียง (ซ้าย)
        .communication_format = I2S_COMM_FORMAT_I2S,         // รูปแบบการสื่อสาร
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,            // ระดับการขัดจังหวะ
        .dma_buf_count = 8,                                  // จำนวนบัฟเฟอร์ DMA
        .dma_buf_len = bufferSize,                           // ขนาดบัฟเฟอร์ DMA
        .use_apll = false,                                   // ไม่ใช้ APLL
        .tx_desc_auto_clear = false,                         // ไม่ล้างอัตโนมัติ
        .fixed_mclk = 0                                      // ไม่กำหนด MCLK
    };

    // การตั้งค่าพิน I2S
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,        // พิน Bit Clock
        .ws_io_num = I2S_WS,          // พิน Word Select
        .data_out_num = I2S_PIN_NO_CHANGE,  // ไม่ใช้พินส่งข้อมูล
        .data_in_num = I2S_SD         // พินรับข้อมูล
    };

    // ติดตั้งไดรเวอร์ I2S
    esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("ข้อผิดพลาดในการติดตั้งไดรเวอร์ I2S: %d\n", result);
        return;
    }

    // ตั้งค่าพิน I2S
    result = i2s_set_pin(I2S_PORT, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("ข้อผิดพลาดในการตั้งค่าพิน I2S: %d\n", result);
        return;
    }

    // ล้างบัฟเฟอร์ DMA
    i2s_zero_dma_buffer(I2S_PORT);
    
    Serial.println("ตั้งค่า I2S เสร็จสิ้น");
}

// ===============================
// ฟังก์ชันเริ่มต้น SD Card
// ===============================
bool setupSDCard() {
    Serial.println("กำลังตรวจสอบ SD Card...");
    
    // เริ่มต้นการใช้งาน SD Card
    if (!SD.begin()) {
        Serial.println("ไม่สามารถเริ่มต้น SD Card ได้");
        M5.Lcd.println("SD Card Error!");
        return false;
    }

    // ตรวจสอบประเภทของ SD Card
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("ไม่พบ SD Card");
        M5.Lcd.println("No SD Card!");
        return false;
    }

    // แสดงข้อมูล SD Card
    Serial.print("ประเภท SD Card: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    // แสดงขนาด SD Card
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("ขนาด SD Card: %lluMB\n", cardSize);

    // แสดงพื้นที่ว่าง
    uint64_t freeSpace = (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
    Serial.printf("พื้นที่ว่าง: %lluMB\n", freeSpace);

    return true;
}

// ===============================
// ฟังก์ชันสร้างชื่อไฟล์ใหม่
// ===============================
String generateFileName() {
    String fileName = "/audio_";      // ชื่อไฟล์เริ่มต้น
    fileName += String(fileCounter);  // เพิ่มตัวเลข
    fileName += ".wav";               // นามสกุลไฟล์
    fileCounter++;                    // เพิ่มตัวนับ
    return fileName;
}

// ===============================
// ฟังก์ชันเขียน WAV Header
// ===============================
void writeWAVHeader(File &file, uint32_t dataSize) {
    WAVHeader header;                           // สร้างโครงสร้าง header
    header.fileSize = dataSize + 36;            // ขนาดไฟล์รวม
    header.dataSize = dataSize;                 // ขนาดข้อมูลเสียง
    
    file.write((uint8_t*)&header, sizeof(header));  // เขียน header ลงไฟล์
}

// ===============================
// ฟังก์ชันเริ่มบันทึกเสียง
// ===============================
void startRecording() {
    if (isRecording) return;                    // ถ้าอยู่ในสถานะบันทึกแล้ว ให้ออกจากฟังก์ชัน
    
    Serial.println("เริ่มบันทึกเสียง...");
    
    // ตรวจสอบพื้นที่ว่างใน SD Card
    uint64_t freeSpace = SD.totalBytes() - SD.usedBytes();
    uint32_t estimatedFileSize = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8 * RECORD_TIME;
    
    if (freeSpace < estimatedFileSize + 1024) {  // เหลือพื้นที่น้อยกว่าขนาดไฟล์ที่คาดว่าจะได้
        Serial.println("พื้นที่ใน SD Card ไม่เพียงพอ");
        M5.Lcd.println("SD Card Full!");
        autoRecordingEnabled = false;            // หยุดการบันทึกอัตโนมัติ
        return;
    }

    // สร้างชื่อไฟล์ใหม่
    String fileName = generateFileName();
    Serial.println("กำลังสร้างไฟล์: " + fileName);
    
    // เปิดไฟล์สำหรับเขียน
    audioFile = SD.open(fileName, FILE_WRITE);
    if (!audioFile) {
        Serial.println("ไม่สามารถสร้างไฟล์ได้");
        return;
    }

    // เขียน WAV header (ชั่วคราว)
    writeWAVHeader(audioFile, 0);

    // เริ่มบันทึก
    isRecording = true;
    recordingStartTime = millis();
    
    // แสดงสถานะบนหน้าจอ
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(50, 50);
    M5.Lcd.println("RECORDING...");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.println("File: " + fileName);
    M5.Lcd.setCursor(10, 120);
    M5.Lcd.println("Duration: 10 seconds");
    M5.Lcd.setCursor(10, 140);
    M5.Lcd.println("Press BtnA to STOP");
}

// ===============================
// ฟังก์ชันหยุดบันทึกเสียง
// ===============================
void stopRecording() {
    if (!isRecording) return;                   // ถ้าไม่ได้บันทึกอยู่ ให้ออกจากฟังก์ชัน
    
    Serial.println("หยุดบันทึกเสียง...");
    
    // คำนวณขนาดข้อมูลเสียง
    uint32_t dataSize = audioFile.size() - sizeof(WAVHeader);
    
    // ย้ายตำแหน่งไปที่จุดเริ่มต้นของไฟล์
    audioFile.seek(0);
    
    // เขียน WAV header ที่ถูกต้อง
    writeWAVHeader(audioFile, dataSize);
    
    // ปิดไฟล์
    audioFile.close();
    
    // เปลี่ยนสถานะ
    isRecording = false;
    
    Serial.println("บันทึกเสียงเสร็จสิ้น");
    
    // แสดงสถานะบนหน้าจอ
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(50, 50);
    M5.Lcd.println("SAVED!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.printf("Files recorded: %d", fileCounter);
    M5.Lcd.setCursor(10, 120);
    M5.Lcd.println("Next recording in 1 min");
    
    // อัพเดทเวลาบันทึกล่าสุด
    lastRecordingTime = millis();
}

// ===============================
// ฟังก์ชันประมวลผลการบันทึก
// ===============================
void processRecording() {
    if (!isRecording) return;                   // ถ้าไม่ได้บันทึกอยู่ ให้ออกจากฟังก์ชัน
    
    // ตรวจสอบว่าบันทึกครบเวลาแล้วหรือไม่
    if (millis() - recordingStartTime >= RECORD_TIME * 1000) {
        stopRecording();                        // หยุดบันทึกเมื่อครบเวลา
        return;
    }

    // อ่านข้อมูลเสียงจาก I2S
    i2s_read(I2S_PORT, audioBuffer, bufferSize * sizeof(int16_t), &bytesRead, portMAX_DELAY);
    
    // เขียนข้อมูลลงไฟล์
    if (bytesRead > 0) {
        audioFile.write((uint8_t*)audioBuffer, bytesRead);
    }

    // แสดงเวลาที่เหลือ
    unsigned long elapsed = millis() - recordingStartTime;
    unsigned long remaining = (RECORD_TIME * 1000) - elapsed;
    M5.Lcd.fillRect(10, 160, 200, 20, BLACK);
    M5.Lcd.setCursor(10, 160);
    M5.Lcd.printf("Time left: %.1f sec", remaining / 1000.0);
}

// ===============================
// ฟังก์ชันตรวจสอบการบันทึกอัตโนมัติ
// ===============================
void checkAutoRecording() {
    if (!autoRecordingEnabled) return;         // ถ้าปิดการบันทึกอัตโนมัติ ให้ออกจากฟังก์ชัน
    if (isRecording) return;                   // ถ้าอยู่ในสถานะบันทึก ให้ออกจากฟังก์ชัน
    
    // ตรวจสอบว่าถึงเวลาบันทึกหรือไม่
    if (millis() - lastRecordingTime >= RECORDING_INTERVAL * 1000) {
        startRecording();                      // เริ่มบันทึกอัตโนมัติ
    }
}

// ===============================
// ฟังก์ชันแสดงสถานะหน้าจอหลัก
// ===============================
void displayMainScreen() {
    if (isRecording) return;                   // ถ้าอยู่ในสถานะบันทึก ไม่ต้องแสดงหน้าจอหลัก
    
    M5.Lcd.fillScreen(BLACK);                  // ล้างหน้าจอ
    M5.Lcd.setTextColor(WHITE);                // ตั้งสีตัวอักษร
    M5.Lcd.setTextSize(2);                     // ตั้งขนาดตัวอักษร
    M5.Lcd.setCursor(30, 30);                  // ตั้งตำแหน่งเคอร์เซอร์
    M5.Lcd.println("Audio Recorder");          // แสดงชื่อโปรแกรม
    
    M5.Lcd.setTextSize(1);                     // เปลี่ยนขนาดตัวอักษร
    M5.Lcd.setCursor(10, 70);                  // ตั้งตำแหน่งใหม่
    M5.Lcd.printf("Files recorded: %d", fileCounter);  // แสดงจำนวนไฟล์ที่บันทึก
    
    M5.Lcd.setCursor(10, 90);                  // ตั้งตำแหน่งใหม่
    if (autoRecordingEnabled) {                // ถ้าเปิดการบันทึกอัตโนมัติ
        unsigned long nextRecording = RECORDING_INTERVAL * 1000 - (millis() - lastRecordingTime);
        M5.Lcd.printf("Next recording: %.1f sec", nextRecording / 1000.0);
    } else {
        M5.Lcd.println("Auto recording: OFF");  // แสดงสถานะปิด
    }
    
    // แสดงข้อมูล SD Card
    M5.Lcd.setCursor(10, 110);                 // ตั้งตำแหน่งใหม่
    uint64_t freeSpace = (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
    M5.Lcd.printf("SD Free: %lluMB", freeSpace);
    
    // แสดงคำแนะนำการใช้งาน
    M5.Lcd.setCursor(10, 140);                 // ตั้งตำแหน่งใหม่
    M5.Lcd.println("BtnA: Manual Record");     // คำแนะนำปุ่ม A
    M5.Lcd.setCursor(10, 160);                 // ตั้งตำแหน่งใหม่
    M5.Lcd.println("BtnB: Toggle Auto");       // คำแนะนำปุ่ม B
    M5.Lcd.setCursor(10, 180);                 // ตั้งตำแหน่งใหม่
    M5.Lcd.println("BtnC: Stop All");          // คำแนะนำปุ่ม C
}

// ===============================
// ฟังก์ชันเริ่มต้นโปรแกรม
// ===============================
void setup() {
    // เริ่มต้น M5CoreS3
    M5.begin();                                // เริ่มต้นระบบ M5CoreS3
    
    // เริ่มต้น Serial สำหรับ debug
    Serial.begin(115200);                      // เริ่มต้นการสื่อสาร Serial
    Serial.println("M5CoreS3 Audio Recorder Starting...");
    
    // ตั้งค่าหน้าจอ
    M5.Lcd.begin();                            // เริ่มต้นหน้าจอ LCD
    M5.Lcd.setRotation(1);                     // หมุนหน้าจอ
    M5.Lcd.fillScreen(BLACK);                  // ล้างหน้าจอด้วยสีดำ
    M5.Lcd.setTextColor(WHITE);                // ตั้งสีตัวอักษรเป็นสีขาว
    M5.Lcd.setTextSize(1);                     // ตั้งขนาดตัวอักษร
    M5.Lcd.setCursor(10, 10);                  // ตั้งตำแหน่งเคอร์เซอร์
    M5.Lcd.println("Initializing...");         // แสดงข้อความเริ่มต้น
    
    // ตั้งค่า SD Card
    if (!setupSDCard()) {                      // ตรวจสอบและเริ่มต้น SD Card
        M5.Lcd.println("SD Card initialization failed!");
        while (1) {                            // หยุดโปรแกรมถ้า SD Card ไม่พร้อม
            delay(1000);                       // รอ 1 วินาที
        }
    }
    
    // ตั้งค่า I2S
    setupI2S();                                // เริ่มต้นระบบ I2S
    
    // จองพื้นที่สำหรับบัฟเฟอร์เสียง
    audioBuffer = (int16_t*)malloc(bufferSize * sizeof(int16_t));
    if (!audioBuffer) {                        // ตรวจสอบการจองพื้นที่
        Serial.println("ไม่สามารถจองพื้นที่สำหรับบัฟเฟอร์ได้");
        M5.Lcd.println("Memory allocation failed!");
        while (1) {                            // หยุดโปรแกรมถ้าจองพื้นที่ไม่ได้
            delay(1000);                       // รอ 1 วินาที
        }
    }
    
    // ตั้งค่าเวลาเริ่มต้น
    lastRecordingTime = millis();              // บันทึกเวลาปัจจุบัน
    
    // แสดงหน้าจอหลัก
    displayMainScreen();                       // แสดงหน้าจอหลัก
    
    Serial.println("ระบบพร้อมใช้งาน");
}

// ===============================
// ฟังก์ชันหลักของโปรแกรม
// ===============================
void loop() {
    // อัพเดทสถานะปุ่ม
    M5.update();                               // อัพเดทสถานะปุ่มกด
    
    // ตรวจสอบการกดปุ่ม A (บันทึกด้วยตนเอง/หยุดบันทึก)
    if (M5.BtnA.wasPressed()) {                // ถ้ากดปุ่ม A
        if (isRecording) {                     // ถ้าอยู่ในสถานะบันทึก
            stopRecording();                   // หยุดบันทึก
        } else {                               // ถ้าไม่ได้บันทึก
            startRecording();                  // เริ่มบันทึก
        }
    }
    
    // ตรวจสอบการกดปุ่ม B (เปิด/ปิดการบันทึกอัตโนมัติ)
    if (M5.BtnB.wasPressed()) {                // ถ้ากดปุ่ม B
        autoRecordingEnabled = !autoRecordingEnabled;  // สลับสถานะการบันทึกอัตโนมัติ
        Serial.printf("การบันทึกอัตโนมัติ: %s\n", autoRecordingEnabled ? "เปิด" : "ปิด");
        if (!isRecording) {                    // ถ้าไม่ได้บันทึกอยู่
            displayMainScreen();               // แสดงหน้าจอหลักใหม่
        }
    }
    
    // ตรวจสอบการกดปุ่ม C (หยุดทุกอย่าง)
    if (M5.BtnC.wasPressed()) {                // ถ้ากดปุ่ม C
        if (isRecording) {                     // ถ้าอยู่ในสถานะบันทึก
            stopRecording();                   // หยุดบันทึก
        }
        autoRecordingEnabled = false;          // ปิดการบันทึกอัตโนมัติ
        Serial.println("หยุดการทำงานทั้งหมด");
        displayMainScreen();                   // แสดงหน้าจอหลัก
    }
    
    // ประมวลผลการบันทึก
    processRecording();                        // ประมวลผลการบันทึกเสียง
    
    // ตรวจสอบการบันทึกอัตโนมัติ
    checkAutoRecording();                      // ตรวจสอบว่าถึงเวลาบันทึกอัตโนมัติหรือไม่
    
    // อัพเดทหน้าจอหลัก (ถ้าไม่ได้บันทึก)
    static unsigned long lastScreenUpdate = 0;
    if (!isRecording && millis() - lastScreenUpdate > 1000) {  // อัพเดททุกๆ 1 วินาที
        displayMainScreen();                   // แสดงหน้าจอหลัก
        lastScreenUpdate = millis();           // บันทึกเวลาอัพเดท
    }
    
    // รอสักครู่เพื่อไม่ให้ CPU ทำงานหนักเกินไป
    delay(10);                                 // รอ 10 มิลลิวินาที
}