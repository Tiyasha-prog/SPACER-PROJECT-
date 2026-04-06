#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Create the BMP280 object
Adafruit_BMP280 bmp; 

// Define the I2C pins for ESP32-WROOM (Standard)
#define I2C_SDA 21
#define I2C_SCL 22

// --- Global Variables for BLE ---
BLECharacteristic *pCharacteristic; 
bool deviceConnected = false;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define PRESSURE_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Callback class to update the 'deviceConnected' status
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEDevice::startAdvertising(); // Restart advertising so you can reconnect
    }
};

void setup() {
  Serial.begin(115200);
  while(!Serial) delay(100); // Wait for serial to connect
  
  Serial.println(F("\n--- ESP32 BMP280 Test ---"));

  // Initialize I2C with specific pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // initialization: Try 0x76 first, then 0x77
  bool status = bmp.begin(0x76); 
  if (!status) {
    status = bmp.begin(0x77);
  }
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor!"));
    Serial.println(F("1. Check wiring (SDA to 21, SCL to 22)"));
    Serial.println(F("2. Check if sensor is BMP280 or BME280"));
    while (1) delay(10); // Halt operation
  }

  Serial.println(F("Sensor found!"));
  // 3. Initialize BLE (As we did before)
  BLEDevice::init("ESP32_Health_Station");

// 2. BLE Initialization
  BLEDevice::init("ESP32_Pressure_Sensor");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      PRESSURE_UUID,
                      BLECharacteristic::PROPERTY_READ | 
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  pServer->getAdvertising()->start();
  Serial.println("BLE Service is now active and advertising!");
  Serial.println("Waiting for a client connection...");

  // --- Detailed Configuration for Weather Station Use ---
  // These settings reduce noise and self-heating
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void loop() {
    // 1. Read Temperature
    float temp = bmp.readTemperature();
    
    // 2. Read Pressure (returns Pascals, divide by 100 for hPa)
    float pressure = bmp.readPressure() / 100.0F;
   

  // 2. Check if a phone is actually connected
  if (deviceConnected) {
    // 3. Convert the number to a "String" (BLE transmits bytes/text)
    String pressureString = String(pressure);
    
    // 4. Update the "characteristic" folder with the new value
    pCharacteristic->setValue(pressureString.c_str());
    
    // 5. "Notify" the phone that the value has changed
    pCharacteristic->notify();
    
    Serial.print("pressure: ");
    Serial.println(pressureString);
  }

  delay(2000); // Wait 2 seconds before the next reading

    
    // 3. Calculate Altitude
    // 1013.25 is the standard sea level pressure. 
    // For TRUE accuracy, change 1013.25 to the current pressure at sea level for your location (from a weather site).
    float altitude = bmp.readAltitude(1013.25); 

    // --- Output to Serial ---
    Serial.print(F("Temp: "));
    Serial.print(temp);
    Serial.print(F(" °C\t"));

    Serial.print(F("Pressure: "));
    Serial.print(pressure);
    Serial.print(F(" hPa\t"));

    Serial.print(F("Alt: "));
    Serial.print(altitude);
    Serial.println(F(" m"));

    delay(2000); // Wait 2 seconds between readings

    

}





  