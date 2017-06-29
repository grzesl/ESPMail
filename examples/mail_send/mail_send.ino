/*
 Name:		mail_send.ino
 Created:	6/28/2017 1:34:35 PM
 Author:	Grzesiek
*/

#include <ESPMail.h>
#include <ESP8266WiFi.h>

const char* ssid = "your_ssid";
const char* password = "your_password";
ESPMail mail;

void setup_wifi() {
	Serial.print("\nConnecting to ");
	Serial.println(ssid);

	WiFi.persistent(false);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
	}

	Serial.println("\nWiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	
	setup_wifi();
	
	mail.begin();
}

// the loop function runs over and over again until power down or reset
void loop() {

	if (send >= 1)
		return;

	mail.setSubject("from@example.com", "Mail Title");
	mail.addTo("to@example.com");
	mail.setBody("Hello world");
	//mail.enableDebugMode();
	if (mail.send("smtp.server.com", 587, "your_smtp_user", "your_smtp_password") == 0)
	{
		Serial.println("Mail send OK");
	}

	send++;
}
