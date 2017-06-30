# ESPMail
Library for sending emails through SMTP mail server. This software use libquickmail.

## Usage

```console
 ESPMail mail;
 mail.begin();

 mail.setSubject("from@example.com", "EMail Subject");
 mail.addTo("to@example.com");
 mail.addCC("cc@example.com");
 mail.addBCC("bcc@example.com");
	
 mail.addAttachment("test.txt", "This is content of attachment.");
	
 mail.setBody("This is an example e-mail.\nRegards Grzesiek");
 //mail.setHTMLBody("This is an example html <b>e-mail<b/>.\n<u>Regards Grzesiek</u>");
	
 if (mail.send("smtp.server.com", 587, "your_smtp_user", "your_smtp_password") == 0)
 {
 	Serial.println("Mail send OK");
 }
```

## Debug Mode
Add this line before send to show communication with SMTP server.

```console
 (...)
 mail.enableDebugMode();
 mail.send("smtp.server.com", 587, "your_smtp_user", "your_smtp_password")
```