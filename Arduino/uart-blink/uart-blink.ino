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
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <Adafruit_NeoPixel.h>

#define PIN D0

// BLE Service
BLEDfu bledfu;    // OTA DFU service
BLEDis bledis;    // device information
BLEUart bleuart;  // uart over ble
BLEBas blebas;    // battery

const unsigned int MAX_MESSAGE_LENGTH = 64;

// Variables will change:
int ledState = LOW;  // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time LED was updated

// constants won't change:
const long interval = 1000;  // interval at which to blink (milliseconds)

uint16_t current_hue = 0;  // in each loop call, the hue of the led strip color will be increased by 1

Adafruit_NeoPixel pixels(1, PIN, NEO_GRB + NEO_KHZ800);  // neo pixel object that is used to control the LED strip


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

#if CFG_DEBUG
  // Blocking wait for connection when debug mode is enabled via IDE
  while (!Serial) yield();
#endif

  Serial.println("Bluefruit52 BLEUART Example");
  Serial.println("---------------------------\n");

  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behavior, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);  // Check bluefruit.h for supported values
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();

  Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
  Serial.println("Once connected, enter character(s) that you wish to send");

  pixels.begin();
}

void startAdv(void) {
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

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
  Bluefruit.Advertising.setInterval(32, 244);  // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);    // number of seconds in fast mode
  Bluefruit.Advertising.start(0);              // 0 = Don't stop advertising after n seconds
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LED_GREEN, ledState);
  }
  // Forward data from HW Serial to BLEUART
  while (Serial.available()) {
    // Delay to wait for enough input, since we have a limited transmission buffer
    delay(2);

    uint8_t bufOut[MAX_MESSAGE_LENGTH];
    int count = Serial.readBytes(bufOut, sizeof(bufOut));
    bleuart.write(bufOut, count);
  }

  // Forward from BLEUART to HW Serial
  while (bleuart.available()) {
    uint8_t bufIn[MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    uint8_t ascode = (uint8_t)bleuart.read();

    Serial.println(ascode);

    //Message coming in (check not terminating character) and guard for over message size
    if (ascode != 'k' && (message_pos < MAX_MESSAGE_LENGTH - 1)) {
      //Add the incoming byte to our message
      bufIn[message_pos] = ascode;

      //Serial.println(message_pos);
      message_pos++;
      delay(10);
    }
    //Full message received...
    else {

      Serial.println("ascode");
      //Print the message (or do other things)
      set_led_colour(bufIn);
      //Reset for the next messag
      bufIn[message_pos] = 0;

      message_pos = 0;
      for (int i = 0; i < message_pos; i++) {
        Serial.print(bufIn[i]);
      }
      Serial.println();
    }
  }
}

// LED colour set must be decimal version of 32bit colour
void set_led_colour(uint8_t bufIn[64]) {
  uint32_t output;
  output = strtoul((const char*)bufIn, nullptr, 10);
  Serial.println(output);
  for (int i = 0; i < 3; i++){
    pixels.setPixelColor(0, output);  // set color of pixel
    pixels.show();
    delay(1000);
    pixels.setPixelColor(0, 0);  // set color of pixel
    pixels.show();
    delay(1000);
  }
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle) {
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
}
