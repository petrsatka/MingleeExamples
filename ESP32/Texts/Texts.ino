#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID for the BLE service
#define SERVICE_UUID                      "00000000-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID  "10000000-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TITLE_VIEW_UUID    "10000001-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TEXT_VIEW_UUID     "10000002-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TEXT_UUID          "10000003-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_PASSWORD_UUID      "10000004-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_PIN_UUID           "10000005-74ee-43ce-86b2-0dde20dcefd6"
// Default UUID mask for the Minglee app is ####face-####-####-####-############
// The segment "face" (case-insensitive) is used by Minglee to identify descriptors
#define CUSTOM_DESCRIPTOR_UUID            "2000face-74ee-43ce-86b2-0dde20dcefd6"

// Custom server callback class to handle connection events
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    Serial.println("Device connected.");
  }

  void onDisconnect(BLEServer *pServer) {
    Serial.println("Device disconnected. Restarting advertising...");
    // Restart advertising when a device disconnects
    BLEDevice::startAdvertising();
  }
};

class StringCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    Serial.print("Received value: ");
    if (!value.isEmpty()) {
      Serial.println(value.c_str());
    }
  }
};

BLECharacteristic *testCharacteristic = NULL;
void setup() {
  Serial.begin(115200);

  // Initialize BLE device with a name
  BLEDevice::init("Minglee device");

  // Configure BLE security settings
  // Static PIN bonding
  BLESecurity *pSecurity = new BLESecurity();
  // It's important to call setStaticPIN() before setAuthenticationMode() for bonding to work correctly
  // Reference: https://github.com/espressif/arduino-esp32/blob/98da424de638836e400d4a110b9cb9a101e8cc22/libraries/BLE/src/BLESecurity.cpp#L65
  pSecurity->setStaticPIN(123456);
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  // Create a BLE server and set its callback class
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create a BLE service with a predefined UUID
  // Service handles by default 15 handles. Each BLE Characteristic takes 2 handles and each BLE Descriptor takes 1 handle.
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 32);  //32 handlers
  // If you add or remove characteristics, it may be necessary to forget the device
  // in the Bluetooth settings and re-pair it on Android for changes to take effect.

  // Create a BLE characteristic for service name
  // The value of this characteristic will be displayed as the service name.
  // The "order" value determines the order in which the service appears in the Minglee app.
  // Only one "serviceName" characteristic is supported per service.
  // If a service contains multiple "serviceName" characteristics, one may be selected randomly.

  BLECharacteristic *pCharacteristicServiceName = pService->createCharacteristic(
    CHARACTERISTIC_SERVICE_NAME_UUID,
    BLECharacteristic::PROPERTY_READ);
  pCharacteristicServiceName->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);

  // Add a custom descriptor used by the Minglee app
  // Only one descriptor matching the mask is supported per characteristic.
  // If multiple descriptors match, one may be selected randomly.
  //! The default maximum length of a descriptor is 100 bytes. Setting a descriptor value that exceeds this limit will cause a crash during startup.
  BLEDescriptor *serviceNameDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID, 200);
  // Set control configuration
  // JSON format. Keys are case-sensitive.
  serviceNameDescriptor->setValue(
    R"({"type":"serviceName", "order":1})");

  pCharacteristicServiceName->addDescriptor(serviceNameDescriptor);

  // Set an initial value for the characteristic
  pCharacteristicServiceName->setValue("Texts");

  //// CONTROLS ////
  // If you add or remove characteristics, it may be necessary to forget the device
  // in the Bluetooth settings and re-pair it on Android for changes to take effect.

  // Title: read-only large text
  BLECharacteristic *pCharacteristicTitleView = pService->createCharacteristic(
    CHARACTERISTIC_TITLE_VIEW_UUID,
    BLECharacteristic::PROPERTY_READ
    //| BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristicTitleView->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
  //! The default maximum length of a descriptor is 100 bytes. Setting a descriptor value that exceeds this limit will cause a crash during startup.
  BLEDescriptor *titleViewDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID, 200);
  titleViewDescriptor->setValue(
    R"({"type":"titleView", "order":1, "disabled":false})"  // Control is always read-only. "Disabled" has only a visual effect.
  );

  pCharacteristicTitleView->addDescriptor(titleViewDescriptor);
  pCharacteristicTitleView->setValue("Large read-only text");

  // TextView: read-only regular-sized text
  BLECharacteristic *pCharacteristicTextView = pService->createCharacteristic(
    CHARACTERISTIC_TEXT_VIEW_UUID,
    BLECharacteristic::PROPERTY_READ
    //| BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristicTextView->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
  //! The default maximum length of a descriptor is 100 bytes. Setting a descriptor value that exceeds this limit will cause a crash during startup.
  BLEDescriptor *textViewDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID, 200);
  textViewDescriptor->setValue(
    R"({"type":"textView", "order":2, "disabled":false})"  // Control is always read-only. "Disabled" has only a visual effect.
  );

  pCharacteristicTextView->addDescriptor(textViewDescriptor);
  pCharacteristicTextView->setValue("Read-only text");

  // Text field: editable control for string characteristics
  // Supports UTF-8 encoding
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  BLECharacteristic *pCharacteristicText = pService->createCharacteristic(
    CHARACTERISTIC_TEXT_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    //| BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristicText->setCallbacks(new StringCharacteristicCallbacks());
  pCharacteristicText->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  //! The default maximum length of a descriptor is 100 bytes. Setting a descriptor value that exceeds this limit will cause a crash during startup.
  BLEDescriptor *textDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID, 200);
  textDescriptor->setValue(
    R"({"type":"text", "order":3, "disabled":false, label:"Text Field Label"})");

  pCharacteristicText->addDescriptor(textDescriptor);
  pCharacteristicText->setValue("Text value");

  // Password field: editable control password string characteristics
  // Supports UTF-8 encoding
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  BLECharacteristic *pCharacteristicPassword = pService->createCharacteristic(
    CHARACTERISTIC_PASSWORD_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    //| BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristicPassword->setCallbacks(new StringCharacteristicCallbacks());
  pCharacteristicPassword->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  //! The default maximum length of a descriptor is 100 bytes. Setting a descriptor value that exceeds this limit will cause a crash during startup.
  BLEDescriptor *passwordDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID, 200);
  passwordDescriptor->setValue(
    R"({"type":"password", "order":4, "disabled":false, label:"Pasword Field Label"})");

  pCharacteristicPassword->addDescriptor(passwordDescriptor);
  pCharacteristicPassword->setValue("");


  // PIN field: editable control PIN string characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  BLECharacteristic *pCharacteristicPIN = pService->createCharacteristic(
    CHARACTERISTIC_PIN_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    //| BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristicPIN->setCallbacks(new StringCharacteristicCallbacks());
  pCharacteristicPIN->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  //! The default maximum length of a descriptor is 100 bytes. Setting a descriptor value that exceeds this limit will cause a crash during startup.
  BLEDescriptor *pinDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID, 200);
  pinDescriptor->setValue(
    R"({"type":"pin", "order":5, "disabled":false, label:"PIN Field Label"})");

  pCharacteristicPIN->addDescriptor(pinDescriptor);
  pCharacteristicPIN->setValue("");

  ////NOTIFY TEST////
  //testCharacteristic = pCharacteristicTitleView;
  //testCharacteristic->addDescriptor(new BLE2902());
  /////
  // Start the BLE service
  pService->start();

  // Start BLE advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // Advertise the service UUID
  pAdvertising->setScanResponse(true);         // Enable scan response
  BLEDevice::startAdvertising();

  Serial.println("BLE server is running and advertising...");
}

int counter = 0;
void loop() {
  // Empty loop since BLE server runs in the background
  //NOTIFY TEST
  /*delay(5000);
  testCharacteristic->setValue(String(counter++));
  testCharacteristic->indicate();*/
  ///////////
}