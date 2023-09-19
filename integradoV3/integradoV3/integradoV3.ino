//Programa para la transmitir lecturas de los sensores a un LoRa32-Oled receptor
//Usando el protocolo de comunicación LoRa
#include <ArduinoJson.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>
#include <Ticker.h>
#include <DHTesp.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <LoRa.h>
#include <SPI.h>

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 23
#define SCREEN_WIDTH 128   // Ancho del display OLED
#define SCREEN_HEIGHT 64   // Alto del display OLED
#define SerialMonitor Serial
//Se definen los pines que serán usados en el módulo transeptor LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
//433E6 para Asia, 866E6 para Europa, 915E6 para América
#define BAND 915E6 
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors
Ticker temperatur;
Ticker humiditi;
Ticker sensation;
Ticker gyrosxyz;
Ticker accxyz;
Ticker altura;

// Crea una instancia de objetos Adafruit
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;
Adafruit_BMP085 bmp;

int pinDHT = 25;
DHTesp dht;

//Variables para manejo de datos LoRa
int readingID = 0;
int counter = 0;
String LoRaMessage = "";

sensors_event_t a, g, temp;
float gyroX, gyroY, gyroZ;
float accX, accY, accZ;
float temperature;

//Gyroscope sensor deviation
float gyroXerror = 0.07;
float gyroYerror = 0.03;
float gyroZerror = 0.01;

// Variable JSON para mantener las lecturas de los sensores
JSONVar readings;

float alturaInicial = 0.0;
char letter;
int timeSinceLastRead = 0;
void getGyroReadings() {
  mpu.getEvent(&a, &g, &temp);

  float gyroX_temp = g.gyro.x;
  if (abs(gyroX_temp) > gyroXerror)  {
    gyroX += gyroX_temp / 50.00;
  }

  float gyroY_temp = g.gyro.y;
  if (abs(gyroY_temp) > gyroYerror) {
    gyroY += gyroY_temp / 70.00;
  }

  float gyroZ_temp = g.gyro.z;
  if (abs(gyroZ_temp) > gyroZerror) {
    gyroZ += gyroZ_temp / 90.00;
  }

  readings["giX"] = String(gyroX);
  readings["giY"] = String(gyroY);
  readings["giZ"] = String(gyroZ);
}

void getAccReadings() {
  mpu.getEvent(&a, &g, &temp);
  // Get current acceleration values
  accX = a.acceleration.x;
  accY = a.acceleration.y;
  accZ = a.acceleration.z;
  readings["aX"] = String(accX);
  readings["aY"] = String(accY);
  readings["aZ"] = String(accZ);
}

void getAltura() {
  float altitude = bmp.readAltitude();
  readings["altitude"] = String(altitude);//BPM180
}

void getTemperatura(){
  //Obtenemos el arreglo de datos (humedad y temperatura)
  TempAndHumidity data = dht.getTempAndHumidity();
  readings["temperatura"] = String(data.temperature, 2);//°C
}

void getHumedad(){  
  //Obtenemos el arreglo de datos (humedad y temperatura)
  TempAndHumidity data = dht.getTempAndHumidity();
  //Mostramos los datos de la temperatura y humedad
  readings["humedad"] = String(data.humidity, 1);//%
}

void getSensacionTermica(){  
  if(timeSinceLastRead > 2000) { 
      //Obtenemos el arreglo de datos (humedad y temperatura)
      TempAndHumidity data = dht.getTempAndHumidity();
      float sensation = dht.computeHeatIndex(data.temperature, data.humidity);
      //Mostramos los datos de la temperatura y humedad
      readings["sensation"] = String(sensation, 1);//sensasion termica en °C
      timeSinceLastRead = 0;
  }
  delay(100);
  timeSinceLastRead += 100;
}

void startDHT(){  
      dht.setup(pinDHT, DHTesp::DHT22);
      TempAndHumidity data = dht.getTempAndHumidity();  
      float humedityTem = data.humidity;
      float temperatureTem = data.temperature;
      if(!isnan(humedityTem) && !isnan(temperatureTem)){               
            temperatur.attach(.1, getTemperatura);
            humiditi.attach(.1, getHumedad);
            sensation.attach(.1, getSensacionTermica);
      }
}

void startMPU(){
  if (mpu.begin()) {
    delay(250);
    gyrosxyz.attach(.1, getGyroReadings);
    accxyz.attach(.1, getAccReadings);    
  }
}

void startBMP() {
  if (bmp.begin()) {
    delay(450);
    alturaInicial = bmp.readAltitude();
    altura.attach(.1, getAltura);
  }
}

//Initialize OLED display
void startOLED(){
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER");
}

void startLoRA(){
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //configuración del módulo transeptor
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Incrementa el readingID cada nueva lectura
    readingID++;
    Serial.println("Starting LoRa failed!"); 
  }
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

void setup() {
  Serial.begin(115200);
  startOLED();
  startLoRA();
  startDHT();
  startMPU();
  startBMP();
}

void loop() {
  LoRa.beginPacket();
  LoRa.print(JSON.stringify(readings));
  LoRa.endPacket();
  
  display.clearDisplay();
  display.setCursor(10, 10);
  display.println(JSON.stringify(readings));
    // Muestra el mensaje en el display
  display.display();
  
  writeString(JSON.stringify(readings));
  Serial.println();  // Start a new line
  if(Serial.available())
  {         
    letter=Serial.read();
    Serial.print(letter);
  }
}

void writeString(String stringData) { // Used to serially push out a String with Serial.write()

  for (int i = 0; i < stringData.length(); i++)

  {

    Serial.write(stringData[i]);   // Push each char 1 by 1 on each loop pass

  }

}// end writeString
