

/*
ESPMail - Library for sending emails based on libquickmail.
Created by Grzegorz Leœniak, June 28, 2017.

ESPMail is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ESPMail is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ESPMail.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef ESPMail_h
#define ESPMail_h

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAM)
#include "Arduino.h"
#include <Client.h>
#include "libquickmail\quickmail.h"
#else
#error Only Arduino MKR1000, Yun, Uno/Mega/Due with either WiFi101 or Ethernet shield. ESP8266 also supported.
#endif

class ESPMail
{
public:
	bool begin()
	{
		quickmail_initialize();		
	}

	void enableDebugMode()
	{
		quickmail_set_debug_log(mailobj, (FILE*)1);
	}

	void setSubject(char *from, char *subject)
	{
		mailobj = quickmail_create(from, subject);
	}

	void addTo(char *to)
	{
		quickmail_add_to(mailobj, to);
	}

	void addCC(char *cc)
	{
		quickmail_add_cc(mailobj, cc);
	}

	void setBody(char *body)
	{
		quickmail_set_body(mailobj, body);
	}

	int send(char *smtpServer, int smtpPort, char *smtpUser, char *smtpPass )
	{
		int res = 0;
		const char* errmsg;
		if ((errmsg = quickmail_send(mailobj, smtpServer, smtpPort, smtpUser, smtpPass)) != NULL)
		{
			Serial.print("Error sending e-mail: ");
			Serial.print(errmsg);
			Serial.print("\n");
			res = 1;
		}
		quickmail_destroy(mailobj);		
		return res;
	}

	~ESPMail()
	{
		quickmail_cleanup();
	}



private:
	quickmail mailobj;
};



#endif //ESPMail_h