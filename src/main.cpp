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
char gpsTime[100];
char gpsDate[100];
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


void drawBatteryLevel(TFT_eSprite *sprite, int x, int y, float voltage)
{
	uint32_t color1 = TFT_GREEN;
	uint32_t color2 = TFT_WHITE;
	uint32_t color3 = TFT_BLUE;
	uint32_t color4 = TFT_RED;
	if (voltage > 4.33)
	{
		(*sprite).fillRect(x, y, 30, 10, color3);
	}
	else if (voltage > 3.2)
	{
		(*sprite).fillRect(x, y, map((long)(voltage * 100), 320, 430, 0, 30), 10, color1);
		(*sprite).setCursor(x + 7, y + 1);
		(*sprite).setTextColor(TFT_DARKGREY);
		(*sprite).printf("%02ld%%", map((long)(voltage * 100), 320, 432, 0, 100));
	}
	else
	{
		(*sprite).fillRect(x, y, 30, 10, color4);
	}
	(*sprite).drawRect(x, y, 30, 10, color2);
}

void print_activity(int angle, double mySpeed)
{

	TFT_eSprite drawingSprite = TFT_eSprite(&tft);
	drawingSprite.setColorDepth(8);
	drawingSprite.createSprite(tft.width(), tft.height());
	
	drawingSprite.fillSprite(TFT_BLACK);
			drawingSprite.setTextSize(1);
		drawingSprite.setTextFont(1);
		drawingSprite.setTextColor(TFT_GREEN);
		drawingSprite.setTextDatum(MC_DATUM);
	drawingSprite.setCursor(0, 0);

	uint16_t v = analogRead(ADC_PIN);
	float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
		drawingSprite.setTextColor(TFT_RED);
	drawingSprite.printf("Voltage : ");
		drawingSprite.setTextColor(TFT_WHITE);
	drawingSprite.printf("%.2fv\n\n", battery_voltage);
	drawBatteryLevel(&drawingSprite, 100, 00, battery_voltage);

	drawingSprite.setCursor(0, 15);
	drawingSprite.setTextColor(TFT_RED);
	drawingSprite.printf("Satellites : ");
	drawingSprite.setTextColor(TFT_WHITE);
	drawingSprite.printf("%ld\n\n", nbre_sat);





	if (nbre_sat < 3)
	{
		drawingSprite.setTextSize(2);
		drawingSprite.setTextFont(1);
		drawingSprite.setTextColor(TFT_GREEN);
		drawingSprite.setTextDatum(MC_DATUM);
		drawingSprite.setCursor(0, 100);
		drawingSprite.printf(" Creation\ndu contact\n avec les\nIlluminatis");
		drawingSprite.setTextColor(TFT_DARKGREY);
		drawingSprite.setTextSize(1);
		drawingSprite.setCursor(0, 230);
		drawingSprite.printf("juthomas.fr");

		drawingSprite.pushSprite(0, 0);
		drawingSprite.deleteSprite();
		return;
	}
	drawingSprite.setCursor(0, 30);
	drawingSprite.setTextColor(TFT_RED);
	drawingSprite.printf("Gps Time : ");
	drawingSprite.setTextColor(TFT_WHITE);
	drawingSprite.print(gpsTime);
	drawingSprite.setCursor(0, 45);
	drawingSprite.setTextColor(TFT_RED);
	drawingSprite.printf("Gps Date : ");
	drawingSprite.setTextColor(TFT_WHITE);
	drawingSprite.print(gpsDate);

	TFT_eSprite direction = TFT_eSprite(&tft);
	direction.setColorDepth(8);
	direction.createSprite(100, 20);
	direction.fillRect(5, 5, 90, 10, TFT_BLUE);
	direction.fillTriangle(90,0,100,10,90,20,TFT_BLUE);
	direction.setPivot(50, 10);
	// direction.setRotation(60);
	
	
	// direction.pushSprite(50, 50);
	TFT_eSprite directionBack = TFT_eSprite(&tft);
	directionBack.setColorDepth(8);
	directionBack.createSprite(100, 100);

	direction.pushRotated(&directionBack, angle+90);
	directionBack.pushToSprite(&drawingSprite, 15, 70);

	drawingSprite.setCursor(0, 185);
	drawingSprite.setTextSize(1);
	drawingSprite.setTextColor(TFT_GREEN);
	drawingSprite.printf(" Vitesse de rotation :");


	drawingSprite.setTextSize(2);
	drawingSprite.setTextFont(1);
	drawingSprite.setTextColor(TFT_GREEN);
	drawingSprite.setTextDatum(MC_DATUM);
	drawingSprite.setCursor(0, 200);
	drawingSprite.printf("%0.2lfkm/h", mySpeed);
	drawingSprite.setTextColor(TFT_DARKGREY);
	drawingSprite.setTextSize(1);
	drawingSprite.setCursor(0, 230);
	drawingSprite.printf("juthomas.fr");


	drawingSprite.pushSprite(0, 0);
	drawingSprite.deleteSprite();

	// drawingSprite.setPivot(0, 0);
	// drawingSprite.pushRotated(60);

}


void loop() {
	static double mySpeed = 0;
	static bool isCalibrated = false;
	static int calMinX = 0;
	static int calMaxX = 0;
	static int calMinY = 0;
	static int calMaxY = 0;



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
			sprintf(gpsDate, "%02d/%02d/%d", gps.date.day(), gps.date.month(), gps.date.year());
			// gpsTime = gps.time.value();
			// gpsTime = (gps.time.hour() + 1 % 24) + ":" + gps.time.minute() + ":" + gps.time.second();
			sprintf(gpsTime, "%02d:%02d:%02d" , (gps.time.hour() + 1 % 24), gps.time.minute(), gps.time.second());

			Serial.println("-------- FIX GPS ------------");
			Serial.print("LATITUDE="); Serial.println(latitude);
			Serial.print("LONGITUDE="); Serial.println(longitude);
			Serial.print("ALTITUDE (m) ="); Serial.println(alt);
			Serial.print("VITESSE (km/h)="); Serial.println(vitesse);
			Serial.print("NOMBRE SATELLITES="); Serial.println(nbre_sat);
			Serial.print("Time="); Serial.println(gpsTime);
			Serial.print("Time2="); Serial.println(gps.time.value());
			Serial.print("Acceleration de la terre="); Serial.println(((gps.date.year() - 2021) * 0.0002));
			Serial.print("VITESSE DE LA TERRE="); Serial.println(2 * PI * cos(radians(latitude)) * (6378 + alt * 0.001) / 24 + vitesse + ((gps.date.year() - 2021) * 0.0002));
			mySpeed = 2 * PI * cos(radians(latitude)) * (6378 + alt * 0.001) / 24 + vitesse + ((gps.date.year() - 2021) * 0.0002);
		}
	}


	magnetic_captor.getHeading(&mx, &my, &mz);
	
	if (isCalibrated == false)
	{
		calMinX = mx;
		calMaxX = mx;
		calMinY = my;
		calMaxY = my;
		isCalibrated = true;
	}
	else
	{
		if (mx < calMinX)
			calMinX = mx;
		else if (mx > calMaxX)
			calMaxX = mx;
		if (my < calMinY)
			calMinY = my;
		else if (my > calMaxY)
			calMaxY = my;
	}

	// affichage des données
	// Serial.println("-------------");
	// Serial.print("mag:\t");
	// Serial.print(mx);
	// Serial.print("\t");
	// Serial.print(my);
	// Serial.print("\t");
	// Serial.print(mz);
	// Serial.println("\t");
 
	// calcul et affichage de l'angle en degrés par rapport au nord

	int xcal = map(mx, calMinX, calMaxX, -1000, 1000);
	int ycal = map(my, calMinY, calMaxY, -1000, 1000);

	// int xcal = map(mx, -950, -150, -1000, 1000);
	// int ycal = map(my, -260, 380, -1000, 1000);
	
	angle = atan2((double)xcal, (double)ycal);
	// Serial.print("angle:\t");
	// Serial.println(angle * 180 / PI);
	print_activity(-(int)(angle * 180 / PI), mySpeed);
	// delay(1000);
}