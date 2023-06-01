#include <errno.h> /* strerror() */
#include <fcntl.h> /* O_RDWR */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h> /* write(), read(), close() */

#define BUFFER_SIZE 256

int openSerialPort(char pathToPort[]) {

  static int serialPort;
  struct termios serialTerminal;

  serialPort = open(pathToPort, O_RDWR);
  if (serialPort < 0) {
    printf("Error open(): %s\n", strerror(errno));
    return -1;
  }

  if (tcgetattr(serialPort, &serialTerminal) != 0) {
    printf("Error tcgetattr(): %s\n", strerror(errno));
    close(serialPort);
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

  cfsetispeed(&serialTerminal, B115200);
  cfsetospeed(&serialTerminal, B115200);

  if (tcsetattr(serialPort, TCSANOW, &serialTerminal) != 0) {
    printf("Error tcsetattr(): %s\n", strerror(errno));
    close(serialPort);
    return -1;
  }

  return serialPort;
}

int querySerialPort(int serialPort, char *input) {

  time_t timeStart;
  unsigned long idx;
  int bytesRead, bufferFree, returnCode, isEnd,
      bufferSize = sizeof(char) * BUFFER_SIZE; /* initial size in bytes */
  char *txBuffer, *buffer = calloc(BUFFER_SIZE, sizeof(char));
  const char *responseCodes[8] = {"\r\nOK\r\n",    "\r\nCONNECT\r\n",
                                  "\r\nRING\r\n",  "\r\nNO CARRIER\r\n",
                                  "\r\nERROR\r\n", "\r\nNO DIALTONE\r\n",
                                  "\r\nBUSY\r\n",  "\r\nNO ANSWER\r\n"};

  txBuffer = malloc(sizeof(char) * (strlen(input) + 2));
  strncpy(txBuffer, input, sizeof(char) * (strlen(input) + 1));
  strncat(txBuffer, "\r", 2 * sizeof(char));
  if (write(serialPort, txBuffer, sizeof(char) * (strlen(txBuffer))) < 0) {
    printf("Error write(): %s\n", strerror(errno));
    free(txBuffer);
    free(buffer);
    return 1;
  }
  free(txBuffer);

  bytesRead = bufferFree = bufferSize;
  returnCode = isEnd = 0;
  timeStart = time(NULL);
  while (!isEnd) {
    bytesRead =
        read(serialPort, buffer + bufferSize - bufferFree, bufferFree - 1);
    if (bytesRead < 0) {
      printf("Error reading serial port\n");
      returnCode = 1;
      break;
    }

    else if (bytesRead > 0) {
      buffer[bytesRead] = '\0';
      bufferFree -= (bytesRead + 1);

      if (bufferFree == 0) {
        bufferSize += bufferFree = BUFFER_SIZE;
        buffer = realloc(buffer, bufferSize);
      }

      for (idx = 0; idx < sizeof(responseCodes) / sizeof(*responseCodes);
           idx++) {
        if (strstr(buffer, responseCodes[idx])) {
          isEnd = 1;
          break;
        }
      }
    } else if (bytesRead == 0 && time(NULL) - timeStart > 180) {
      printf("Response timeout after 180 seconds\n");
      returnCode = 1;
      break;
    }
  }

  printf("%s\n", buffer);
  free(buffer);
  return returnCode;
}

int main() {

  char *commandList[] = {"AT", "ATI", "AT+COPS?"/* ,
                         "AT+COPS=?" */},
       serialFilePath[] = "/dev/serial0";

  int serialPort;
  unsigned long idx;

  serialPort = openSerialPort(serialFilePath);
  if (serialPort < 0) {
    printf("Error openSerialPort()\n");
    return 1;
  }

  for (idx = 0; idx < sizeof(commandList) / sizeof(*commandList); idx++) {
    if (0 != querySerialPort(serialPort, commandList[idx])) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      return 1;
    }
  }

  close(serialPort);

  return 0;
}
