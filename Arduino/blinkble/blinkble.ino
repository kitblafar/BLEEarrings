/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This sketch implement part of Nordic custom LED Button Service (LBS).
 * Install "nRF Blinky" app on your Android/iOS to control the on-board LED
 * - https://apps.apple.com/us/app/nrf-blinky/id1325014347
 * - https://play.google.com/store/apps/details?id=no.nordicsemi.android.nrfblinky
 */

#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>

// max concurrent connections supported by this example
#define MAX_PRPH_CONNECTION   2
uint8_t connection_count = 0;

/* Nordic Led-Button Service (LBS)
 * LBS Service: 00001523-1212-EFDE-1523-785FEABCD123
 * LBS Button : 00001524-1212-EFDE-1523-785FEABCD123
 * LBS LED    : 00001525-1212-EFDE-1523-785FEABCD123
 */

const uint8_t LBS_UUID_SERVICE[] =
{
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
  0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0x00, 0x00
};

const uint8_t LBS_UUID_CHR_BUTTON[] =
{
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
  0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0x00, 0x00
};

const uint8_t LBS_UUID_CHR_LED[] =
{
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
  0xDE, 0xEF, 0x12, 0x12, 0x25, 0x15, 0x00, 0x00
};

BLEService        lbs(LBS_UUID_SERVICE);
BLECharacteristic lsbButton(LBS_UUID_CHR_BUTTON);
BLECharacteristic lsbLED(LBS_UUID_CHR_LED);

// Use on-board button if available, else use A0 pin
#ifdef PIN_BUTTON1
  uint8_t button = PIN_BUTTON1;
#else
  uint8_t button = A0;
#endif

// CPB button active state is HIGH, all other is low
#ifdef ARDUINO_NRF52840_CIRCUITPLAY
  #define BUTTON_ACTIVE     HIGH
#else
  #define BUTTON_ACTIVE     LOW
#endif

uint8_t buttonState;

#ifdef PIN_NEOPIXEL

#ifndef NEOPIXEL_NUM
#define NEOPIXEL_NUM  1
#endif

Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NEOPIXEL_NUM, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

#endif

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  (void) conn_handle;

  setLED(true);
  lsbLED.write8(0x01);

  connection_count++;
  Serial.print("Connection count: ");
  Serial.println(connection_count);

  // Keep advertising if not reaching max
  if (connection_count < MAX_PRPH_CONNECTION)
  {
    Serial.println("Keep advertising");
    Bluefruit.Advertising.start(0);
  }
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  setLED(false);
  lsbLED.write8(0x00);

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);

  connection_count--;
}

void setLED(uint8_t on)
{
  Serial.print(on);
  // data = 1 -> LED = On
  // data = 0 -> LED = Off
  uint8_t leds[3] = {LED_RED, LED_GREEN, LED_BLUE};
  uint8_t rgb[3] = {0x00, 0x00, 0x00};
  bool b = 0;

  for (int i=0; i<3; i++){
    b = bitRead(on, i+4); // read the 5th bit
    digitalWrite(leds[i], !(b)); // set the required LED on
    if (b = HIGH){
      rgb[i] = 0x20; // set either red blue or green
    }
  }   
  #ifdef PIN_NEOPIXEL
    uint32_t c = neopixel.Color(rgb[0], rgb[1], rgb[2]);
    neopixel.fill(c, 0, NEOPIXEL_NUM);
    neopixel.show();
  #endif
}
   



void led_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len)
{
  (void) conn_hdl;
  (void) chr;
  (void) len; // len should be 1

  setLED(*data);
}

void setup()
{
  pinMode(button, BUTTON_ACTIVE ? INPUT_PULLDOWN : INPUT_PULLUP);
  buttonState = (BUTTON_ACTIVE == digitalRead(button));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1-LED_STATE_ON); // led off

#ifdef PIN_NEOPIXEL
  neopixel.begin();
#endif

  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 nRF Blinky Example");
  Serial.println("------------------------------\n");

  // Initialize Bluefruit with max concurrent connections as Peripheral = MAX_PRPH_CONNECTION, Central = 0
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin(MAX_PRPH_CONNECTION, 0);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Setup the LED-Button service using
  Serial.println("Configuring the LED-Button Service");

  // Note: You must call .begin() on the BLEService before calling .begin() on
  // any characteristic(s) within that service definition.. Calling .begin() on
  // a BLECharacteristic will cause it to be added to the last BLEService that
  // was 'begin()'ed!
  lbs.begin();

  // Configure Button characteristic
  // Properties = Read + Notify
  // Permission = Open to read, cannot write
  // Fixed Len  = 1 (button state)
  lsbButton.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  lsbButton.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  lsbButton.setFixedLen(1);
  lsbButton.begin();
  lsbButton.write8(buttonState);

  // Configure the LED characteristic
  // Properties = Read + Write
  // Permission = Open to read, Open to write
  // Fixed Len  = 1 (LED state)
  lsbLED.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  lsbLED.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  lsbLED.setFixedLen(1);
  lsbLED.begin();
  lsbLED.write8(0x01); // led = on when connected

  lsbLED.setWriteCallback(led_write_callback);

  // Setup the advertising packet(s)
  Serial.println("Setting up the advertising");
  startAdv();
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(lbs);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void loop()
{
  delay(10); // poll button every 10 ms

  uint8_t newState = (BUTTON_ACTIVE == digitalRead(button));

  // only notify if button state changes
  if ( newState != buttonState)
  {
    buttonState = newState;
    lsbButton.write8(buttonState);

    // notify all connected clients
    for (uint16_t conn_hdl=0; conn_hdl < MAX_PRPH_CONNECTION; conn_hdl++)
    {
      if ( Bluefruit.connected(conn_hdl) && lsbButton.notifyEnabled(conn_hdl) )
      {
        lsbButton.notify8(conn_hdl, buttonState);
      }
    }
  }
}


