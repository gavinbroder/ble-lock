// Bluetooth Low Energy Lock
// (c) 2014-2015 Don Coleman
//
// RedBear Lab BLE Shield http://redbearlab.com/bleshield/
// or Bluefruit LE http://adafru.it/1697
// Solenoid Lock http://adafru.it/1512
// arduino-BLEPeripheral https://github.com/sandeepmistry/arduino-BLEPeripheral.git

#include <SPI.h>
#include <BLEPeripheral.h>

#define LOCK_PIN 6
#define RED_LED_PIN 4
#define GREEN_LED_PIN 5

// See BLE Peripheral documentation for setting for your hardware
// https://github.com/sandeepmistry/arduino-BLEPeripheral#pinouts

// BLE Shield 2.x
#define BLE_REQ 9
#define BLE_RDY 8
#define BLE_RST UNUSED

// Adafruit Bluefruit LE
//#define BLE_REQ 10
//#define BLE_RDY 2
//#define BLE_RST 9

BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
BLEService lockService = BLEService("03D0A2C0-9FBA-4350-9B74-47DBEA0CE228");
BLECharacteristic unlockCharacteristic = BLECharacteristic("03D0A2C1-9FBA-4350-9B74-47DBEA0CE228", BLEWrite, 20);
BLECharacteristic statusCharacteristic = BLECharacteristic("03D0A2C2-9FBA-4350-9B74-47DBEA0CE228", BLERead | BLENotify, 20);
//BLECharCharacteristic statusCharacteristic = BLECharCharacteristic("03D0A2C2-9FBA-4350-9B74-47DBEA0CE228", BLERead | BLENotify);

// code that opens the lock
char secret[] = { '1', '2', '3', '4', '5' };
long openTime = 0;

void setup() {
  Serial.begin(9600);
  Serial.println(F("BLE Lock"));

  // set advertised name and service
  blePeripheral.setDeviceName("BLE Lock");
  blePeripheral.setAdvertisedServiceUuid(lockService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(lockService);
  blePeripheral.addAttribute(unlockCharacteristic);
  blePeripheral.addAttribute(statusCharacteristic);

  // assign event handlers for connected, disconnected to peripheral
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  unlockCharacteristic.setEventHandler(BLEWritten, unlockCharacteristicWritten);

  // begin initialization
  blePeripheral.begin();

  pinMode(LOCK_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void loop() {

  // Tell the bluetooth radio to do whatever it should be working on
  blePeripheral.poll();

  // close lock and reset lights after 4 seconds
  if (openTime && millis() - openTime > 4000) {
    resetLock();
  }

}

void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print(F("Connected event, central: "));
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print(F("Disconnected event, central: "));
  Serial.println(central.address());
}

void unlockCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // central wrote new value to the unlock characteristic
  Serial.print(F("Characteristic event, writen: "));

  openLock(characteristic.value());
}

void openLock(const unsigned char* code) {
  openTime = millis();  // set even if bad code so we can reset the lights
  
  // does the code match the secret
  // TODO how do I know the length of characteristic? is it 0 padded to 20?
  boolean match = false;
  for (int i = 0; i < sizeof(code); i++) {
    Serial.print(secret[i]);
    Serial.print(" ");
    Serial.println(code[i]);
    
    if (secret[i] != code[i]) {
      match = false;
      break;
    } else {
      match = true;
    }
  }
  
  if (match) {
    // open the lock
    Serial.println("Code matches, opening lock");
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(LOCK_PIN, HIGH); // open the lock
    statusCharacteristic.setValue("unlocked");
  } else {
    // bad code, don't open
    Serial.println("Invalid code "); // + code);
    digitalWrite(RED_LED_PIN, HIGH);
    statusCharacteristic.setValue("invalid code");
  }
}

// closes the lock and resets the lights
void resetLock() {
  // reset the lights
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(LOCK_PIN, LOW); // close the lock
  statusCharacteristic.setValue("locked");
  openTime = 0;
}
