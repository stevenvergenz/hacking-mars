#include <SPI.h>
#include <BME280SpiSw.h>

#define SERIAL_BAUD 9600
#define CALIBRATION_SAMPLES 100

struct CalibrationData {
  float offset = 0.0f;
  float scale = 1.0f;

  float values[CALIBRATION_SAMPLES];
  unsigned valueCount = 0;
  float valueAvg;
  float valueRange;
};

int calibrateSensors();
void computeOffsetAndScale(CalibrationData* data);

// CS, MOSI, MISO, SCL
BME280SpiSw::Settings settings1(11, 12, 10, 13);
BME280SpiSw bme1(settings1);

BME280SpiSw::Settings settings2(2, 3, 4, 5);
BME280SpiSw bme2(settings2);
unsigned outputMod = 0;

//////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(SERIAL_BAUD);

  while(!Serial) {} // Wait

  //Serial.println("Starting up");
  while(!bme1.begin() || !bme2.begin()) {
    //Serial.println("Could not find BME280 sensor 1 or 2");
    delay(1000);
  }

  /*switch(bme1.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
  switch(bme2.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }*/

  
}

//////////////////////////////////////////////////////////////////
int calibrated = 0;

void loop() {
   if (!calibrated) {
      calibrated = calibrateSensors();
   } else {
      printBME280Data(&Serial);
   }
}

//////////////////////////////////////////////////////////////////
CalibrationData calibration1;
CalibrationData calibration2;

int calibrateSensors() {
  if (calibration1.valueCount < CALIBRATION_SAMPLES) {
    calibration1.values[calibration1.valueCount++] = bme1.pres(BME280::PresUnit_hPa);
  }
  if (calibration2.valueCount < CALIBRATION_SAMPLES) {
    calibration2.values[calibration2.valueCount++] = bme2.pres(BME280::PresUnit_hPa);
  }
  
  if (calibration1.valueCount == CALIBRATION_SAMPLES && calibration2.valueCount == CALIBRATION_SAMPLES) {
    computeOffsetAndScale(&calibration1);
    computeOffsetAndScale(&calibration2);

    float newAvg = (calibration1.valueAvg + calibration2.valueAvg) / 2;
    float newRange = (calibration1.valueRange + calibration2.valueRange) / 2;
    calibration1.offset = newAvg - calibration1.valueAvg;
    calibration2.offset = newAvg - calibration2.valueAvg;
    //calibration1.scale = newRange / calibration1.valueRange;
    //calibration2.scale = newRange / calibration2.valueRange;
    return 1;
  }

  delay(50);
  return 0;
}

void computeOffsetAndScale(CalibrationData* data) {
  float minValue(100000), maxValue(0), sumValues(0);
  for (int i = 0; i < data->valueCount; i++) {
    minValue = min(minValue, data->values[i]);
    maxValue = max(maxValue, data->values[i]);
    sumValues += data->values[i];
  }
  data->valueAvg = sumValues / data->valueCount;
  data->valueRange = maxValue - minValue;
}

//////////////////////////////////////////////////////////////////
void printBME280Data(Stream* client) {
  float pres1(NAN), pres2(NAN);
   
  BME280::PresUnit presUnit(BME280::PresUnit_hPa);

  pres1 = bme1.pres(presUnit) * calibration1.scale + calibration1.offset;
  pres2 = bme2.pres(presUnit) * calibration2.scale + calibration2.offset;

  if (outputMod == 0) {
    client->print(pres1);
    client->print(",");
    client->println(pres2);
  }
  outputMod = (outputMod + 1) % 20;
  delay(50);
}
