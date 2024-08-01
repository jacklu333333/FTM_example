#include "pin_config.h"
#include "TFT_eSPI.h"
#include <SPI.h>
#include <Wire.h>
#include "WiFi.h"
#include "CRC8.h"
#include "CRC.h"

const char PREFIX[] = "WiFi_FTM_Responder";
char WIFI_FTM_SSIDs[100][100];
const char *WIFI_FTM_PASS = "ftm_responder";
int AP_COUNT = 0;

const uint8_t FTM_FRAME_COUNT = 8;
const uint16_t FTM_BURST_PERIOD = 2;
SemaphoreHandle_t ftmSemaphore;
bool ftmSuccess = true;

TFT_eSPI tft = TFT_eSPI();
int x, y = 0;
int counter = 0;

CRC8 crc;

void onFtmReport(arduino_event_t *event)
{
	const char *status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
	wifi_event_ftm_report_t *report = &event->event_info.wifi_ftm_report;
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
		int index = strlen(msg);
		sprintf(msg + index, "CRC:%02X\n", crc.calc());
		Serial.print(msg);

		tft.printf("FTM Estimate: Distance: %.2f m, Return Time: %lu ns\n", (float)report->dist_est / 100.0, report->rtt_est);
		counter++;
		free(report->ftm_report_data);
	}
	else
	{
		tft.print("FTM Error: ");
		tft.println(status_str[report->status]);
	}
	xSemaphoreGive(ftmSemaphore);
}

bool getFtmReport()
{
	if (!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD))
	{
		tft.println("FTM Error: Initiate Session Failed");
		return false;
	}
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
	tft.setTextFont(4);
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(x, y);

	ledcSetup(0, 4000, 8);
	ledcAttachPin(TFT_BL, 0);
	for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle = dutyCycle + 5)
	{
		ledcWrite(0, dutyCycle);
		delay(5);
	}
	delay(3000);

	ftmSemaphore = xSemaphoreCreateBinary();

	WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
	Serial.println("FTM Initiator Example");
}

void loop()
{
	memset(WIFI_FTM_SSIDs, NULL, sizeof(WIFI_FTM_SSIDs));
	AP_COUNT = 0;
	WiFi.scanNetworks();

	for (int i = 0; i < WiFi.scanComplete(); i++)
	{
		if (strncmp(WiFi.SSID(i).c_str(), PREFIX, strlen(PREFIX)) == 0)
		{
			sprintf(WIFI_FTM_SSIDs[i], WiFi.SSID(i).c_str());
			AP_COUNT++;
		}
	}
	for (int i = 0; i < AP_COUNT; i++)
	{
		tft.printf("Connecting to\n%s\n", WIFI_FTM_SSIDs[i]);
		WiFi.begin(WIFI_FTM_SSIDs[i], WIFI_FTM_PASS);
		unsigned long start = millis();
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(50);
			tft.print(".");
			if (millis() - start > 450)
			{
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

		tft.println("WiFi Connected");

		while (getFtmReport())
			;

		while (WiFi.disconnect(false, true) != true)
			;
		counter = 0;
	}
}