#include <time.h>
#include <array>
#include <iterator>
#include <locale>
#include <time.h>

#include <Arduino.h>
#include <painlessMesh.h>
#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>

#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
using namespace std;

#define SEALEVELPRESSURE_HPA (1013.25)
const char* SAVE_FILE = "/measures.txt";
const int HISTO_MAX_SIZE = 24;
const int REFRESH_RATE = 3000;
const float BME_TEMP_LIMIT_MAX = 85.0;
const float BME_TEMP_LIMIT_MIN = -40.0;
File file;

const int I2C_SDA = 21;
const int I2C_SCL = 22;

const int LCD_WIDTH = 128;
const int LCD_HEIGHT = 64;
const int LCD_GRAPH_X = 14;
const int LCD_GRAPH_Y = 14;

Adafruit_BME680 bme;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void drawTempGraph();
void addMeasure();

void setup() {
  Serial.begin(115200);

  while(!Serial);
    Serial.println("Waiting for Serial");
  
  Wire.begin(I2C_SDA, I2C_SCL);

  //Checking if SPIFFS ready
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  if(!u8g2.begin())
    Serial.println(F("OLED screen not found"));

  if(!bme.begin()) {
    Serial.println(F("BME680 sensor not found"));
    return;
  }
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
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

  /*
  u8g2.firstPage();
  int x;
  do {
    drawTempGraph(u8g2);
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0,15,"Hello World!");
  } while ( u8g2.nextPage() );
  */
  delay(REFRESH_RATE);
}

struct MEASURE {
  double temperature = 0.0;
  double humidity = 0.0;
  uint32_t pressure = 0.0;
  uint32_t gaz = 0.0;
};

class Bee {
    public:
      Bee();
      Bee(Adafruit_BME680 b);

      inline float Bee::getTemperature(){
        return this->temperature;
      };

      inline float Bee::getHumidity(){
        return this->humidity;
      }

      inline uint32_t Bee::getPressure(){
        return this->pressure;
      };

      inline uint32_t Bee::getGas(){
        return this->gas_resistance;
      };
    
      inline void Bee::Bee(Adafruit_BME680 b) {
        this->bme = b;
        this->refresh();
      };

      inline void Bee::to_string(){
        Serial.println("-- Sensors values --");
        Serial.print("- Temperature = ");
        Serial.print(this->getTemperature());
        Serial.println("°C");
        Serial.print("- Humidity = ");
        Serial.print(this->getHumidity());
        Serial.println("%");
        Serial.print("- Pressure = ");
        Serial.print(this->getPressure());
        Serial.println("bar");
        Serial.print("- VoC = ");
        Serial.print(this->getGas());
        Serial.println("%");
      };

      /**
       * @brief refresh the Bee values (temperature, humidity, pressure and gaz)
       */
      inline void Bee::refresh(){
        Serial.println("-- Refreshing sensor values --");
        uint32_t gaz = this->bme.gas_resistance / 1000.0;
        uint32_t pres = this->bme.pressure / 100.0;
        float hum = this->bme.humidity;
        float temp = this->bme.temperature;

        this->temperature = temp;
        this->humidity = hum;
        this->pressure = pres;
        this->gas_resistance = gaz;

        MEASURE m;
        m.temperature = temp;
        m.humidity = hum;
        m.pressure = pres;
        m.gaz = gaz;

        this->updateHisto(m);
        this->persist(m);
      };

      /**
       * @brief Get Altitude in meters
       * @return float 
       */
      inline float getAltitude(){
        return this->bme.readAltitude(SEALEVELPRESSURE_HPA);
      };

      /**
       * @brief Scan all the last Temperature measures and return the higher value
       * @return float 
       */
      inline float getTemperatureMax(){
        float max = BME_TEMP_LIMIT_MIN;
        for (auto m : this->histo){
          if(m.temperature > max)
            max = m.temperature;
        }
        return max;
      };

      /**
       * @brief Scan all the last Temperature measures and return the lower value
       * @return float 
       */
      inline float getTemperatureMin(){
        float min = BME_TEMP_LIMIT_MAX;
        for (auto m : this->histo){
          if(m.temperature < min)
            min = m.temperature;
        }
        return min;
      };

    private:
        float temperature;
        float humidity;
        uint32_t pressure;
        uint32_t gas_resistance;
        Adafruit_BME680 bme;
        list<MEASURE> histo;
    
        /**
         * @brief update the measures history. Remove the older one if already full (check HISTO_MAX_SIZE)
         * @param m 
         */
        inline void Bee::updateHisto(MEASURE m){
          //if the list is full, pop the older measure
          if(this->histo.size() == HISTO_MAX_SIZE){
            this->histo.pop_front();
          }

          this->histo.push_back(m);
        }
        
        /**
         * @brief Record measures in ESP32 memory
         * @param m 
         * @return true 
         * @return false 
         */
        inline bool Bee::persist(MEASURE m){
            try{
              file = SPIFFS.open(SAVE_FILE, FILE_WRITE);
              file.print(m.temperature);
              file.print(";");
              file.print(m.humidity);
              file.print(";");
              file.print(m.pressure);
              file.print(";");
              file.print(m.gaz);
              file.println(";");
              file.close();
            }catch(const std::exception& e){
              Serial.print("/!\\ -- EXCEPTION -- /!\\");
              Serial.print(e.what());
              Serial.print("---------------------");
            }
        };

        /**
         * @brief Update the graph values
         * 
         */
        inline void Bee::drawGraph(){
          u8g2.?
        }
};