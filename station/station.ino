/* Wi-Fi FTM Responder Arduino Example

  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/
#include "pin_config.h"
// #include "picoImage.h"
#include "TFT_eSPI.h"
// #include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "WiFi.h"

#define STATION_ID 3
// Change the SSID and PASSWORD here if needed
char WIFI_FTM_SSID[50];

const char *WIFI_FTM_PASS = "ftm_responder";

TFT_eSPI tft = TFT_eSPI();
void setup()
{
	sprintf(WIFI_FTM_SSID, "WiFi_FTM_Responder%d", STATION_ID);

	Serial.begin(115200);
	Serial.println("Starting SoftAP with FTM Responder support");
	// Enable AP with FTM support (last argument is 'true')
	WiFi.softAP(WIFI_FTM_SSID, WIFI_FTM_PASS, 1, 0, 10, true);
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	tft.init();
	tft.setRotation(1);
	tft.setSwapBytes(true);
	// tft.loadFont(FreeMono9pt7bBitmaps);
	tft.setTextFont(4);
	// tft.invertDisplay(false);
	tft.fillScreen(TFT_BLACK);
	// tft.pushImage(0, 0, 240, 135, Lilygo1);
	tft.setCursor(0, 0);
	tft.printf("ID:\n %s\n", WIFI_FTM_SSID);

	ledcSetup(0, 4000, 8);
	ledcAttachPin(TFT_BL, 0);
	for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle = dutyCycle + 5) 
	{
		ledcWrite(0, dutyCycle);
		delay(5);
	}
	delay(3000);
}

void loop()
{
	Serial.println("Looping and Alive");
	delay(100);
}
