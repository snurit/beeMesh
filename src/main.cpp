#include <time.h>
#include <array>
#include <iterator>

#include <Arduino.h>
#include <painlessMesh.h>
#include <Wire.h>
#include <SPI.h>

#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
using namespace std;

#define SEALEVELPRESSURE_HPA (1013.25)

const int I2C_SDA = 21;
const int I2C_SCL = 22;

const int LCD_WIDTH = 128;
const int LCD_HEIGHT = 64;

struct HOURLY_MEASURE {
  double temp = 0.0;
  double min = 0.0;
  double max = 0.0;
  double gaz = 0.0;
};

array<HOURLY_MEASURE, 24> measures;

Adafruit_BME680 bme;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void drawTempGraph();
void addMeasure();

void setup() {
  Serial.begin(115200);
  while(!Serial);
    Serial.println("Waiting for Serial");
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if(!u8g2.begin())
    Serial.println(F("OLED screen not found"));

  if(!bme.begin())
    Serial.println(F("BME680 sensor not found"));
  
  // Preparint & setting BME
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  srand(time(NULL));
}

void drawTempGraph(U8G2 u8g2) {
  int x;
  for(int i=0; i < LCD_WIDTH; i++) {
    x = rand() % LCD_HEIGHT + 1;
    u8g2.drawVLine(i, 0, x);
    Serial.print("- Drawing a line at ");
    Serial.print(i);
    Serial.print(" - ");
    Serial.print(0);
    Serial.print(" - ");
    Serial.println(x);
  }
}

void addMeasure(Adafruit_BME680 bme){
  time_t t = time(NULL);
}

void loop() {
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading BME"));
    return;
  }

  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  Serial.print(F("Reading completed at "));
  Serial.println(millis());

  HOURLY_MEASURE m{bme.temperature, bme.pressure / 100.0, bme.humidity, bme.gas_resistance / 1000.0};

  /*
  Serial.print(F("Approx. Altitude = "));
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(F(" m"));
  */
  

  Serial.println();

  /*
  u8g2.firstPage();
  int x;
  do {
    drawTempGraph(u8g2);
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0,15,"Hello World!");
  } while ( u8g2.nextPage() );
  */
  delay(1000);
}