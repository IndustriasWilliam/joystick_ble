#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include "hid_joystick.h"

BLE_Client_Joystick BLE_Joystick;

void movement(int x, int y)
{
  Serial.printf("move(%d, %d)\r\n", x, y);
}

void button_A(bool pressed)
{
  Serial.print("Button A ");
  if (pressed) {
    Serial.println("pressed");
  }
  else {
    Serial.println("released");
  }
}

void connection(bool connected)
{
  Serial.print("Joystick is ");
  if (!connected) {
    Serial.println("not ");
  }
  Serial.println("connected");
}



void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE HID Joystick client");
  BLE_Joystick.set_connect_callback(connection);
  BLE_Joystick.set_movement_callback(movement);
  BLE_Joystick.set_button_A_callback(button_A);
  BLE_Joystick.begin();
 

}

void loop() {
  BLE_Joystick.loop();
}