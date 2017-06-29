/*
    This file is part of libquickmail.

    libquickmail is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libquickmail is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libquickmail.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef ARDUINO
#include <Client.h>
#include <WiFiClient.h>
#endif

#include "smtpsocket.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <stdarg.h>
#if _MSC_VER
#define va_copy(dst,src) ((dst) = (src))
#endif

////////////////////////////////////////////////////////////////////////

#define DEBUG_ERROR(errmsg)
static const char* ERRMSG_MEMORY_ALLOCATION_ERROR = "Memory allocation error";

////////////////////////////////////////////////////////////////////////

SOCKET socket_open (const char* smtpserver, unsigned int smtpport, char** errmsg)
{
#ifdef ARDUINO
	WiFiClient *cli = new WiFiClient();
	SOCKET	sock = (void*)cli;

	Serial.println("socket_open");

	if (cli->connect(smtpserver, smtpport) == 0)
	{
		*errmsg = "Error connecting to SMTP server";
		Serial.println(*errmsg);
	}
#else
  struct in_addr ipv4addr;
  struct sockaddr_in remote_sock_addr;
  static const struct linger linger_option = {-1, 2};   //linger 2 seconds when disconnecting
  //determine IPv4 address of SMTP server
  ipv4addr.s_addr = inet_addr(smtpserver);
  if (ipv4addr.s_addr == INADDR_NONE) {
    struct hostent* addr;
    if ((addr = gethostbyname(smtpserver)) != NULL && (addr->h_addrtype == AF_INET && addr->h_length >= 1 && ((struct in_addr*)addr->h_addr)->s_addr != 0))
      memcpy(&ipv4addr, addr->h_addr, sizeof(ipv4addr));
  }
  if (ipv4addr.s_addr == INADDR_NONE) {
    if (errmsg)
      *errmsg = "Unable to resolve SMTP server host name";
    return INVALID_SOCKET;
  }
  //create socket
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    if (errmsg)
      *errmsg = "Error creating socket for SMTP connection";
    return INVALID_SOCKET;
  }
  //connect
  remote_sock_addr.sin_family = AF_INET;
  remote_sock_addr.sin_port = htons(smtpport);
  remote_sock_addr.sin_addr.s_addr = ipv4addr.s_addr;
  if (connect(sock, (struct sockaddr*)&remote_sock_addr, sizeof(remote_sock_addr)) != 0) {
    if (errmsg)
      *errmsg = "Error connecting to SMTP server";
    socket_close(sock);
    return INVALID_SOCKET;
  }
  //set linger option
  setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&linger_option, sizeof(linger_option));
#endif
  return sock;
}

void socket_close (SOCKET sock)
{
#ifdef ARDUINO
	delete ((WiFiClient*)sock);
#elif defined _WIN32
  shutdown(sock, 2);
#else
  closesocket(sock);
#endif
}

int socket_send (SOCKET sock, const char* buf, int len)
{
#ifdef ARDUINO
	int total_sent = 0;
	WiFiClient *cli = (WiFiClient*)sock;
	
	/*Serial.printf("socket_send: %d ", len);
	for (int i = 0; i < len; i++)
	{
		Serial.printf("%c", buf[i], len);
	}
	Serial.printf("\n");*/
	total_sent = cli->write(buf, len);
	return total_sent;
#else

  int total_sent = 0;
  int l = 0;
  if (sock == 0 || !buf)
    return 0;
  if (len < 0)
    len = strlen(buf);
  while (len > 0 && (l = send(sock, buf, len, 0)) < len) {
    if (l == SOCKET_ERROR || l > len)
      return (total_sent > 0 ? total_sent : -1);
    total_sent += l;
    buf += l;
    len -= l;
  }
  return total_sent + l;
#endif
}

int socket_data_waiting (SOCKET sock, int timeoutseconds)
{
#ifdef ARDUINO
	WiFiClient *cli = (WiFiClient*)sock;
	int avaliable;
	int ms = millis();

	do
	{
		avaliable = cli->available();
		if (avaliable > 0)
			break;

		if ((millis() - ms) / 1000 > timeoutseconds)
			break;

	} while (1);

	//Serial.printf("socket_data_waiting: %d ", avaliable);

	return avaliable?1:0;

#else
  fd_set rfds;
  struct timeval tv;
  if (sock == 0)
    return 0;
  //make a set with only this socket
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  //make a timeval with the supplied timeout
  tv.tv_sec = timeoutseconds;
  tv.tv_usec = 0;
  //check the socket
  return (select(1, &rfds, NULL, NULL, &tv) > 0);
#endif
}

int socket_receive(SOCKET sock, char * buff, int len, int flags)
{
#ifdef ARDUINO
	int r;
	WiFiClient *cli = (WiFiClient*)sock;
	r = cli->read((unsigned char*)buff, len);
	//Serial.printf("socket_receive: %x %c %d r:%d\n", *buff, *buff, len, r);
	//Serial.printf("socket_receive avai: %d\n", cli->available());
	return r;

#else
	return recv(sock, buff, len, flags);
#endif

}

char* socket_receive_smtp (SOCKET sock)
{
  char* buf = NULL;
  int bufsize = READ_BUFFER_CHUNK_SIZE;
  int pos = 0;
  int linestart;
  int n;
  if ((buf = (char*)malloc(bufsize)) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  do {
    //insert line break if response is multiple lines
    if (pos > 0) {
      buf[pos++] = '\n';
      if (pos >= bufsize) {
        char* newbuf;
        if ((newbuf = (char*)realloc(buf, bufsize + READ_BUFFER_CHUNK_SIZE)) == NULL) {
          free(buf);
          DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
          return NULL;
        }
        buf = newbuf;
        bufsize += READ_BUFFER_CHUNK_SIZE;
      }
    }
    //add each character read until it is a line break
    linestart = pos;
    while ((n = socket_receive(sock, buf + pos, 1, 0)) == 1) {
      //detect optional carriage return (CR)
      if (buf[pos] == '\r')
        if (socket_receive(sock, buf + pos, 1, 0) < 1)
          return NULL;
      //detect line feed (LF)
      if (buf[pos] == '\n')
        break;
      //enlarge buffer if necessary
      if (++pos >= bufsize) {
        char* newbuf;
        if ((newbuf = (char*)realloc(buf, bufsize + READ_BUFFER_CHUNK_SIZE)) == NULL) {
          free(buf);
          DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
          return NULL;
        }
        buf = newbuf;
        bufsize += READ_BUFFER_CHUNK_SIZE;
      }
    }
    //exit on error (e.g. if connection is closed)
    if (n < 1)
      return NULL;
  } while (!isdigit(buf[linestart]) || !isdigit(buf[linestart + 1]) || !isdigit(buf[linestart + 2]) || buf[linestart + 3] != ' ');
  buf[pos] = 0;
  return buf;
}

int socket_get_smtp_code (SOCKET sock, char** message)
{
  int code;
  char* buf = socket_receive_smtp(sock);

  //Serial.printf("socket_get_smtp_code: %s\n", buf);

  if (!buf || strlen(buf) < 4 || (buf[3] != ' ' && buf[3] != '-')) {
    free(buf);
    return 999;
  }
  //get code
  buf[3] = 0;
  code = atoi(buf);
  //get error message (if needed)
  if (message /*&& code >= 400*/)
    *message = strdup(buf + 4);
  //clean up and return
  free(buf);
  return code;
}

int socket_smtp_command(SOCKET sock, FILE* debuglog, const char* templ, ...)
{
	char* message;
	int statuscode;
	//send command (if one is supplied)
	if (templ) {
		va_list ap;
		//va_list aq;
		char tmp[260];
		char* cmd = tmp;
		int cmdlen = 256;
		
		va_start(ap, templ);
		//construct command to send
		//va_copy(aq, ap);
		//cmdlen = vsnprintf(tmp, 256, templ, aq);
		//va_end(aq);
		//if ((cmd = (char*)malloc(cmdlen + 3)) == NULL) {
		//	DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
		//		if (debuglog)
//#ifdef ARDUINO
		//			Serial.printf("%s\n", ERRMSG_MEMORY_ALLOCATION_ERROR);
//#else
		//			fprintf(debuglog, "%s\n", ERRMSG_MEMORY_ALLOCATION_ERROR);
//#endif
					
			//va_end(ap);
			//return 999;
		//}
		cmdlen = vsnprintf(cmd, cmdlen + 1, templ, ap);

		va_end(ap);
		//log command to send
		if (debuglog)
#ifdef ARDUINO
			Serial.print("SMTP> ");
			Serial.print(cmd);
			Serial.print("\n");
#else
			fprintf(debuglog, "SMTP> %s\n", cmd);
#endif
		//append CR+LF
		strcpy(cmd + cmdlen, "\r\n");
		cmdlen += 2;
		//send command
		statuscode = socket_send(sock, cmd, cmdlen);
		//clean up
		//free(cmd);
		
		if (statuscode < 0)
			return 999;
	}

	socket_data_waiting(sock, 1);
	//receive result
	message = NULL;
	statuscode = socket_get_smtp_code(sock, &message);
	if (debuglog)
#ifdef ARDUINO
		Serial.print("SMTP< ");
		Serial.print(statuscode);
		Serial.print(" ");
		Serial.print((message ? message : ""));
		Serial.print("\n");
#else
		fprintf(debuglog, "SMTP< %i %s\n", statuscode, (message ? message : ""));
#endif
		
	free(message);
	return statuscode;
}
