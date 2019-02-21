#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"
#include <SoftwareSerial.h>

#define MODE_LED_BEHAVIOUR "MODE"

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);
Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN, BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

void setup(void)
{
  ble.begin(false);
}

void loop(void)
{
  // Check for data
  String data = String("");

  while ( ble.available() )
  {
    int c = ble.read();
    data += (char)c;
  }

  if ( data != "" ) 
  {
    // Echo back to bluefruit
    ble.print("Got " + data);
  }
}
