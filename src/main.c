#include <bits/types/time_t.h>
#include <errno.h> /* strerror() */
#include <fcntl.h> /* O_RDWR */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h> /* write(), read(), close() */

int openSerialPort(char pathToPort[], int baudRate) {

  static int serialPort;
  struct termios serialTerminal;

  serialPort = open(pathToPort, O_RDWR);
  if (serialPort < 0) {
    printf("Error open(): %s\n", strerror(errno));
    return -1;
  }

  if (tcgetattr(serialPort, &serialTerminal) != 0) {
    printf("Error tcgetattr(): %s\n", strerror(errno));
    return -1;
  }

  serialTerminal.c_cflag |= CREAD;

  /* serialTerminal.c_iflag &= ~(BRKINT | IGNBRK | INLCR | IGNCR | ICRNL | IXOFF
     | IUTF8 | IUCLC | IXANY | IMAXBEL); */
  serialTerminal.c_iflag &=
      ~(IGNBRK | BRKINT | INLCR | IGNCR | ICRNL | IXON | IXOFF | IUCLC | IXANY |
        IMAXBEL | IGNPAR | PARMRK | INPCK | ISTRIP);

  /* serialTerminal.c_lflag &= ~(ECHO | ECHONL | NOFLSH | ISIG | TOSTOP | ICANON
     | IEXTEN | ECHOE | ECHOK); */
  serialTerminal.c_lflag &= ~(ICANON | ISIG | ECHO | ECHONL | ECHOE | ECHOK);

  /* serialTerminal.c_oflag &=
      ~(OLCUC | OCRNL | OPOST | OFILL | ONLCR | ONOCR | ONLRET | OFDEL); */
  serialTerminal.c_oflag &= ~(OPOST);

  serialTerminal.c_cc[VTIME] = 0;
  serialTerminal.c_cc[VMIN] = 0;

  cfsetispeed(&serialTerminal, baudRate);
  cfsetospeed(&serialTerminal, baudRate);

  if (tcsetattr(serialPort, TCSANOW, &serialTerminal) != 0) {
    printf("Error tcsetattr(): %s\n", strerror(errno));
    close(serialPort);
    return -1;
  }

  return serialPort;
}

int readSerialPort(int serialPort) {

  time_t start;
  int bytesRead, bufferFree, idx, returnCode, isEnd;
  int bufferSize = sizeof(char) * 256; /* initial size in bytes */
  char *buffer = calloc(256, sizeof(char));
  const char *responseCodes[8] = {"\r\nOK\r\n",    "\r\nCONNECT\r\n",
                                  "\r\nRING\r\n",  "\r\nNO CARRIER\r\n",
                                  "\r\nERROR\r\n", "\r\nNO DIALTONE\r\n",
                                  "\r\nBUSY\r\n",  "\r\nNO ANSWER\r\n"};

  bytesRead = bufferFree = bufferSize;
  returnCode = isEnd = 0;
  start = time(NULL);

  while (!isEnd) {
    if (bufferFree < 1) {
      bufferSize += sizeof(char) * 256;
      buffer = realloc(buffer, bufferSize);
      if (buffer == NULL) {
        printf("Error realloc(): %s\n", strerror(errno));
        returnCode = 1;
        break;
      }
    }
    bytesRead = read(serialPort, buffer + bufferSize - bufferFree, bufferFree);
    if (bytesRead < 0) {
      printf("Error read(): %s\n", strerror(errno));
      returnCode = 1;
      break;
    } else if (bytesRead > 0) {
      bufferFree -= bytesRead;
      start = time(NULL);
    } else if (bytesRead == 0 && difftime(time(NULL), start) > 180) {
      printf("Response timeout\n");
      returnCode = 1;
      break;
    } else {
      for (idx = 0; (long unsigned)idx <
                    sizeof(responseCodes) / sizeof(responseCodes[1]);
           idx++) {
        if (strstr(buffer, responseCodes[idx]) != NULL) {
          isEnd = 1;
          break;
        }
      }
    }
  }

  printf("%s", buffer);
  free(buffer);
  return returnCode;
}

int main() {

  int serialPort;

  serialPort = openSerialPort("/dev/serial0", 115200);

  if (write(serialPort, "AT\r", sizeof("AT\r")) < 0) {
    printf("Error write(): %s\n", strerror(errno));
    close(serialPort);
    return 1;
  }
  readSerialPort(serialPort);

  if (write(serialPort, "ATI\r", sizeof("ATI\r")) < 0) {
    printf("Error write(): %s\n", strerror(errno));
    close(serialPort);
    return 1;
  }
  readSerialPort(serialPort);

  if (write(serialPort, "AT+COPS?\r", sizeof("AT+COPS?\r")) < 0) {
    printf("Error write(): %s\n", strerror(errno));
    close(serialPort);
    return 1;
  }
  readSerialPort(serialPort);

  close(serialPort);

  return 0;
}
