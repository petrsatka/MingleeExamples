#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID for the BLE service
#define SERVICE_UUID        "00000001-74ee-43ce-86b2-0dde20dcefd6"
// UUID for the BLE characteristic
#define CHARACTERISTIC_UUID "10000001-74ee-43ce-86b2-0dde20dcefd6"
// Default UUID mask for the Minglee app is ####face-####-####-####-############
// The segment "face" (case-insensitive) is used by Minglee to identify descriptors
#define CUSTOM_DESCRIPTOR_UUID  "2000face-74ee-43ce-86b2-0dde20dcefd6"

// Custom server callback class to handle connection events
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Device connected.");
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Device disconnected. Restarting advertising...");
        // Restart advertising when a device disconnects
        BLEDevice::startAdvertising();
    }
};

void setup() {
    Serial.begin(115200);

    // Initialize BLE device with a name
    BLEDevice::init("Minglee device");

    // Configure BLE security settings
    BLESecurity *pSecurity = new BLESecurity();
    // No bonding (ESP_LE_AUTH_NO_BOND)
    // Set ESP_LE_AUTH_REQ_SC_MITM_BOND to enable bonding
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_NO_BOND);
    
    // Uncomment the following lines to enable PIN-based bonding
    // pSecurity->setCapability(ESP_IO_CAP_IO);
    // pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    // pSecurity->setStaticPIN(123456);

    // Create a BLE server and set its callback class
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create a BLE service with a predefined UUID
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE characteristic with read and notify properties
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

    // Uncomment the following line to enable encrypted access to the characteristic
    // pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

    // Add a custom descriptor used by the Minglee app
    BLEDescriptor *customDescriptor = new BLEDescriptor(CUSTOM_DESCRIPTOR_UUID);
    //Set control configuration
    customDescriptor->setValue(
R"(
{
  "type":"title"
}
)");
    pCharacteristic->addDescriptor(customDescriptor);

    // Set an initial value for the characteristic
    pCharacteristic->setValue("This Is Title");

    // Start the BLE service
    pService->start();

    // Start BLE advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID); // Advertise the service UUID
    pAdvertising->setScanResponse(true);       // Enable scan response
    BLEDevice::startAdvertising();

    Serial.println("BLE server is running and advertising...");
}

void loop() {
    // Empty loop since BLE server runs in the background
}
