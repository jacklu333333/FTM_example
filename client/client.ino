/* Wi-Fi FTM Initiator Arduino Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "pin_config.h"
// #include "picoImage.h"
#include "TFT_eSPI.h"
#include <SPI.h>
#include <Wire.h>
#include "WiFi.h"
#include "CRC8.h"
#include "CRC.h"

/*
   THIS FEATURE IS SUPPORTED ONLY BY ESP32-S2 AND ESP32-C3
*/

// Change the SSID and PASSWORD here if needed
const char PREFIX[] = "WiFi_FTM_Responder";
// const char *WIFI_FTM_SSIDs[] = {"WiFi_FTM_Responder0", "WiFi_FTM_Responder1", "WiFi_FTM_Responder2", "WiFi_FTM_Responder3"};
char WIFI_FTM_SSIDs[100][100];
const char *WIFI_FTM_PASS = "ftm_responder"; // STA Password
// const int AP_COUNT = sizeof(WIFI_FTM_SSIDs) / sizeof(WIFI_FTM_SSIDs[0]);
int AP_COUNT = 0;

// FTM settings
// Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0 (No pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 8;
// Requested time period between consecutive FTM bursts in 100’s of milliseconds (allowed values - 0 (No pref) or 2-255)
const uint16_t FTM_BURST_PERIOD = 2;
// Semaphore to signal when FTM Report has been received
SemaphoreHandle_t ftmSemaphore;
// Status of the received FTM Report
bool ftmSuccess = true;

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h
int x, y = 0;
int counter = 0;

// crc
CRC8 crc;

// FTM report handler with the calculated data from the round trip
// WARNING: This function is called from a separate FreeRTOS task (thread)!
void onFtmReport(arduino_event_t *event)
{
	const char *status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
	wifi_event_ftm_report_t *report = &event->event_info.wifi_ftm_report;
	// Set the global report status
	ftmSuccess = report->status == FTM_STATUS_SUCCESS;
	if (ftmSuccess)
	{
		tft.fillScreen(TFT_BLACK);
		tft.setTextSize(1);
		tft.setCursor(0, 0);
		char msg[150];
		memset(msg, 0, 100);
		crc.reset();

		sprintf(msg, "ID:%s Distance:%.2f Time:%lu ", WiFi.SSID().c_str(), (float)report->dist_est / 100.0, report->rtt_est);
		crc.add((uint8_t *)msg, strlen(msg));
		// add crc to the end of the message
		int index = strlen(msg);
		sprintf(msg + index, "CRC:%02X\n", crc.calc());
		Serial.print(msg);

		tft.printf("FTM Estimate: Distance: %.2f m, Return Time: %lu ns\n", (float)report->dist_est / 100.0, report->rtt_est);
		// Serial.printf("ID %s\n Distance: %f", report->bssid, (float)report->dist_est / 100.0);
		// The estimated distance in meters may vary depending on some factors (see README file)
		// Serial.printf("FTM Estimate: Distance: %.2f m, Return Time: %lu ns\n", (float)report->dist_est / 100.0, report->rtt_est);
		// Pointer to FTM Report with multiple entries, should be freed after use
		counter++;
		free(report->ftm_report_data);
	}
	else
	{
		// Serial.print("FTM Error: ");
		// Serial.println(status_str[report->status]);
		tft.print("FTM Error: ");
		tft.println(status_str[report->status]);
	}
	// Signal that report is received
	xSemaphoreGive(ftmSemaphore);
}

// Initiate FTM Session and wait for FTM Report
bool getFtmReport()
{
	if (!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD))
	{
		// Serial.println("FTM Error: Initiate Session Failed");
		tft.println("FTM Error: Initiate Session Failed");
		return false;
	}
	// Wait for signal that report is received and return true if status was success
	return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess;
}

void setup()
{
	Serial.begin(115200);
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
	tft.setCursor(x, y);
	// tft.printf("Connecting to %s\n", WIFI_FTM_SSID);

	ledcSetup(0, 4000, 8);
	ledcAttachPin(TFT_BL, 0);
	for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle = dutyCycle + 5) // 逐渐点亮
	{
		ledcWrite(0, dutyCycle);
		delay(5);
	}
	delay(3000);

	// Create binary semaphore (initialized taken and can be taken/given from any thread/ISR)
	ftmSemaphore = xSemaphoreCreateBinary();

	// Will call onFtmReport() from another thread with FTM Report events.
	WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
	Serial.println("FTM Initiator Example");
}

void loop()
{
	memset(WIFI_FTM_SSIDs, NULL, sizeof(WIFI_FTM_SSIDs));
	AP_COUNT = 0;
	// // wifi scanning
	WiFi.scanNetworks();
	// // serial print the number of wifi networks
	// Serial.printf("Number of networks found: %d\n", WiFi.scanComplete());
	// // print out the name one by one
	for (int i = 0; i < WiFi.scanComplete(); i++)
	{
		// Serial.printf("Network %d: %s\n", i, );

		if (strncmp(WiFi.SSID(i).c_str(), PREFIX, strlen(PREFIX)) == 0)
		{
			sprintf(WIFI_FTM_SSIDs[i], WiFi.SSID(i).c_str());
			AP_COUNT++;
		}
	}
	// Serial.println("-----------------------------------------------------");
	// Serial.println("Scanning for WiFi FTM Responders");
	for (int i = 0; i < AP_COUNT; i++)
	{
		// Serial.printf("%s:\n", WIFI_FTM_SSIDs[i]);
		tft.printf("Connecting to\n%s\n", WIFI_FTM_SSIDs[i]);
		WiFi.begin(WIFI_FTM_SSIDs[i], WIFI_FTM_PASS);
		unsigned long start = millis();
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(50);
			// Serial.print(".");
			tft.print(".");
			if (millis() - start > 450)
			{
				// Serial.println("WiFi Connection Timeout");
				tft.println("WiFi Connection Timeout");
				break;
			}
		}
		if (WiFi.status() != WL_CONNECTED)
		{
			tft.fillScreen(TFT_BLACK);
			tft.setCursor(0, 0);
			continue;
		}

		// Serial.println("");
		// Serial.println("WiFi Connected");
		tft.println("WiFi Connected");

		// Serial.print("Initiating FTM session with Frame Count ");
		// Serial.print(FTM_FRAME_COUNT);
		// Serial.print(" and Burst Period ");
		// Serial.print(FTM_BURST_PERIOD * 100);
		// Serial.println(" ms");
		while (getFtmReport())
			;
		// print how much time it takes
		// Serial.printf("    Time elapsed: %lu ms access time %d\n", millis() - start, counter);
		counter = 0;
		while (WiFi.disconnect(false, true) != true)
			;
	}
}