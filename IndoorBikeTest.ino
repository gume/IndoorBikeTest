#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

//https://os.mbed.com/users/tenfoot/code/BLE_CycleSpeedCadence/file/7e334e81da21/CyclingSpeedAndCadenceService.h/
https://www.bluetooth.com/specifications/gatt/characteristics/

bool BleIF_deviceConnected;
bool BleIF_pDeviceConnected;

class GattService {
public:
  enum {
     UUID_CYCLING_SPEED_AND_CADENCE = 0x1816,
     UUID_FTMS = 0x1826
  };
};

class GattCharacteristic {
public:
  enum {
    UUID_CSC_MEASUREMENT_CHAR = 0x2A5B,
    UUID_CSC_FEATURE_CHAR = 0x2A5C,
    UUID_SENSOR_LOCATION_CHAR = 0x2A5D,
    UUID_SC_CONTROL_POINT_CHAR = 0x2A55,

    UUID_FTMS_FEATURE = 0x2ACC, // Read
    UUID_INDOOR_BIKE_DATA = 0x2AD2, // Notify
    UUID_TRAINING_STATUS = 0x2AD3, // Read, Notify
    UUID_RESISTANCE_LEVEL_RANGE = 0x2AD6, // Read
    UUID_FTMS_CONTROL_POINT = 0x2AD9, // Write, Indicate
    UUID_FTMS_STATUS = 0x2ADA // Notify
  };
};

class BleIF_ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Connected.");
    BleIF_deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    BleIF_deviceConnected = false;
    Serial.println("Disconnected.");
  }
};

class BleIF_CCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    for (int i = 0; i < value.length(); i++) {
      Serial.print(value[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");

    if (value.length() < 1) return;
    uint8_t req = value[0];

    uint8_t resp[20];
    resp[0] = 0x80;
    resp[1] = req;
    uint8_t respl;

    switch (req) {
      case 0:   // Control request
      case 1:   // Reset
      case 4:   // Resistance control
      case 5:   // Power control
        resp[2] = 0x01; // Success
        respl = 3;
        break;
      default:
        resp[2] = 0x02; // Not supported
        respl = 3;
        break;
    }
    
  }
};

BLECharacteristic *pFtmsFeature, *pBikeData, 
  *pTrainingStatus, *pResistanceLevelRange, 
  *pFtmsControlPoint, *pFtmsStatus;

uint32_t c = 0;

void setup() {
  Serial.begin(500000);
  
  BLEDevice::init("Testbike");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleIF_ServerCallbacks());

  BLEService *pService = pServer->createService((uint16_t)GattService::UUID_FTMS);
  
  pFtmsFeature = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_FTMS_FEATURE,
             BLECharacteristic::PROPERTY_READ);
  // Support for Cadence, Resistance Level
  // Support for Resistance Target, Training Time, Bike Simulation, Cadence
  uint8_t vFF[] = { 2 + 128, 0, 0, 0, 4, 2 + 4 + 32, 1, 0 };  
  pFtmsFeature->setValue((uint8_t*)&vFF, 8);

  pBikeData = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_INDOOR_BIKE_DATA,
             BLECharacteristic::PROPERTY_NOTIFY);           
  pBikeData->addDescriptor(new BLE2902());
  
  pResistanceLevelRange = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_RESISTANCE_LEVEL_RANGE,
             BLECharacteristic::PROPERTY_READ);
  int16_t vRLR[] = { 0, 100, 5 }; // Resistance 0.0 - 10.0 with 0.5 increment
  pResistanceLevelRange->setValue((uint8_t*)&vRLR, 6);
             
  pFtmsControlPoint = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_FTMS_CONTROL_POINT,
             BLECharacteristic::PROPERTY_WRITE | 
             BLECharacteristic::PROPERTY_INDICATE);
  pFtmsControlPoint->setCallbacks(new BleIF_CCallbacks());

  pFtmsStatus = pService->createCharacteristic(
             (uint16_t)GattCharacteristic::UUID_FTMS_STATUS,
             BLECharacteristic::PROPERTY_NOTIFY);
  pFtmsStatus->addDescriptor(new BLE2902());
  
  pService->start();               

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  BLEAdvertisementData ftmsData = BLEAdvertisementData();
  char serviceData[] = {1, 32, 0}; // Available, Indoor Bike (+ termination)
  //ftmsData.setServiceData((uint16_t)GattService::UUID_FTMS, serviceData);
  ftmsData.setServiceData((uint16_t)GattService::UUID_FTMS, std::string(serviceData, 3));
  pAdvertising->setAdvertisementData(ftmsData);
  pAdvertising->addServiceUUID((uint16_t)GattService::UUID_FTMS);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

void BleIF_update(uint16_t cad, uint16_t res, uint16_t pwr) {
  // Cadence is in 0.5 resolution
  // Resistance in 1 resolution  
  // Power in 1 resolution
  uint8_t v[8];
  v[0] = 1 + 4 + 32 + 64; // Cadence, Resistance, Power
  v[1] = 0;
  v[2] = cad % 256;
  v[3] = cad / 256;
  v[4] = res % 256;
  v[5] = res / 256;
  v[6] = pwr % 256;
  v[7] = pwr / 256;  
  
  pBikeData->setValue(v, 8);
  pBikeData->notify();
}

void loop() {
  Serial.println(c);
  int i;
  for (i=0; i<5; i++) {
    BleIF_update(120, 4, 60);
    delay(1000);
  }
  for (i=0; i<5; i++) {
    BleIF_update(140, 4, 70);
    delay(1000);
  }
  for (i=0; i<5; i++) {
    BleIF_update(199, 3, 90);
    delay(1000);
  }
}
