/*
 Sending emails example.
 Name:		mail_send.ino
 Created:	6/28/2017 1:34:35 PM
*/

#include <WiFi.h>
#include <ESPMail.h>

const char* ssid = "your_ssid";
const char* password = "your_password";
ESPMail mail;
int send = 1;

void setup_wifi() {
	Serial.print("\nConnecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(500);
	}

	Serial.println("\nWiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void setup() {
	Serial.begin(115200);
	
	setup_wifi();
	
	mail.begin();
}

void loop() {

	if (send == 0)
		return;

	mail.setSubject("from@example.com", "EMail Subject");
	mail.addTo("to@example.com");
	mail.addCC("cc@example.com");
	mail.addBCC("bcc@example.com");
	
	mail.addAttachment("test.txt", "This is content of attachment.");
	
	mail.setBody("This is an example e-mail.\nRegards Grzesiek");
	//mail.setHTMLBody("This is an example html <b>e-mail<b/>.\n<u>Regards Grzesiek</u>");
	
	//mail.enableDebugMode();
	if (mail.send("smtp.server.com", 587, "your_smtp_user", "your_smtp_password") == 0)
	{
		Serial.println("Mail send OK");
	}

	send = 0;
}