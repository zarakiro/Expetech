/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   This example file shows how to calibrate the load cell and optionally store the calibration
   value in EEPROM, and also how to change the value manually.
   The result value can then later be included in your project sketch or fetched from EEPROM.

   To implement calibration in your project sketch, follow the simplified procedure as follows:
       LoadCell.tare();
       //place known mass
       LoadCell.refreshDataSet();
       float newCalibrationValue = LoadCell.getNewCalibration(known_mass);
*/

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Pins:
const int HX711_dout = 4; // MCU > HX711 dout pin
const int HX711_sck = 5; // MCU > HX711 sck pin

// HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAddress = 0;
unsigned long t = 0;

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  // LoadCell.setReverseOutput(); // Uncomment to turn a negative output value to positive
  unsigned long stabilizingTime = 2000; // Precision right after power-up can be improved by adding a few seconds of stabilizing time
  boolean tareFlag = true; // Set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingTime, tareFlag);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0); // User set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  calibrate(); // Start calibration procedure
}

void loop() {
  static boolean newDataReady = false;
  const int serialPrintInterval = 0; // Increase value to slow down serial print activity

  // Check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print(i);
      Serial.println(",");
      newDataReady = false;
      t = millis();
    }
  }

  // Receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); // Tare
    else if (inByte == 'r') calibrate(); // Calibrate
    else if (inByte == 'c') changeSavedCalFactor(); // Edit calibration value manually
  }

  // Check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell on a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean resumeFlag = false;
  while (resumeFlag == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      resumeFlag = true;
    }
  }

  Serial.println("Now, place your known mass on the load cell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float knownMass = 0;
  resumeFlag = false;
  while (resumeFlag == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      knownMass = Serial.parseFloat();
      if (knownMass != 0) {
        Serial.print(knownMass);
        Serial.println(",");
        resumeFlag = true;
      }
    }
  }

  LoadCell.refreshDataSet(); // Refresh the dataset to ensure that the known mass is measured correctly
  float newCalibrationValue = LoadCell.getNewCalibration(knownMass); // Get the new calibration value

  Serial.print(newCalibrationValue);
  Serial.println(",");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual editing of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}

void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean resumeFlag = false;
  Serial.println("***");
  Serial.print(oldCalibrationValue);
  Serial.println(",");
  Serial.println("Now, send the
