#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include <WiFiUdp.h>
#include <TinyGPS++.h>
#include <HMC5883L.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL              4   // Display backlight control pin
#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0



int vref = 1100;

#define BLACK 0x0000
#define WHITE 0xFFFF
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);


char data;
double latitude;
double longitude;
double alt; //altitude
double vitesse;
unsigned long nbre_sat;

TinyGPSPlus gps;

HardwareSerial Serial3(1);


HMC5883L magnetic_captor;
int16_t mx, my, mz; // déclaration des variables sur les axes x, y, z
float angle;


void showVoltage()
{
	static uint64_t timeStamp = 0;
	if (millis() - timeStamp > 1000) {
		timeStamp = millis();
		uint16_t v = analogRead(ADC_PIN);
		float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
		String voltage = "Voltage :" + String(battery_voltage) + "V";
		Serial.println(voltage);
		tft.fillScreen(TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
	}
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial3.begin(9600, SERIAL_8N2, 26, 27);
	Wire.begin();

	pinMode(ADC_EN, OUTPUT);
	digitalWrite(ADC_EN, HIGH);
	tft.init();
	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);



	if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
			pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
			digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
	}

	tft.setTextSize(1);
	tft.setTextColor(TFT_GREEN);
	tft.setCursor(0, 0);
	tft.setTextDatum(MC_DATUM);

	tft.drawString("Connecting", tft.width() / 2, tft.height() / 2);

  magnetic_captor.initialize();
  delay(1000);
  while (!magnetic_captor.testConnection()) {
    Serial.println("erreur connexion capteur HMC5883L..");
    delay(500);
  }

	Serial.print("hello\n");
	Serial.print(".\n");

}



void loop() {
	// Serial.print(".\n");
	delay(100);
	while (Serial3.available()) {
    data = Serial3.read();
    // Serial.print(data);
    gps.encode(data);
    if (gps.location.isUpdated() || gps.satellites.isUpdated())
    {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      alt = gps.altitude.meters();
      vitesse = gps.speed.kmph();
      nbre_sat = gps.satellites.value();
 
      Serial.println("-------- FIX GPS ------------");
      Serial.print("LATITUDE="); Serial.println(latitude);
      Serial.print("LONGITUDE="); Serial.println(longitude);
      Serial.print("ALTITUDE (m) ="); Serial.println(alt);
      Serial.print("VITESSE (km/h)="); Serial.println(vitesse);
      Serial.print("NOMBRE SATTELLITES="); Serial.println(nbre_sat);
    }
  }
	magnetic_captor.getHeading(&mx, &my, &mz);
 
  // affichage des données
  Serial.println("-------------");
  Serial.print("mag:\t");
  Serial.print(mx);
  Serial.print("\t");
  Serial.print(my);
  Serial.print("\t");
  Serial.print(mz);
  Serial.println("\t");
 
  // calcul et affichage de l'angle en degrés par rapport au nord
  angle = atan2((double)mx, (double)my);
  Serial.print("angle:\t");
  Serial.println(angle * 180 / PI);
  delay(1000);
}