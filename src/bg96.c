#include "gpio.h"
#include "serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int powerOn(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin) {
  int moduleStatus, serialPort;
  char *response;
  time_t pwrKeyTime;

  moduleStatus = checkPinVal(pathToChip, statusPin);
  if (0 > moduleStatus) {
    printf("Error checkPinVal()\n");
    return 1;
  } else if (moduleStatus == 1) {
    printf("Module is already on\n");
    return 1;
  } else if (0 != modemPowerToggle(pathToChip, pwrKeyPin)) {
    printf("Error toggling modem power\n");
    return 1;
  }
  pwrKeyTime = time(NULL);
  while (!checkPinVal(pathToChip, statusPin)) {
    sleep(1);
    if (time(NULL) - pwrKeyTime > 10) {
      printf("Error: Modem status pin did not switch on before 10s\n");
      return 1;
    }
  }

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    return 1;
  }
  response = malloc(sizeof(char));
  if (0 != querySerialPort(&response, serialPort, "AT", 10)) {
    printf("Error querying serial port\n");
    free(response);
    close(serialPort);
    return 1;
  }
  if (NULL == strstr(response, "\r\nOK\r\n")) {
    printf("Module responded not OK. Response:\n%s\n", response);
    free(response);
    close(serialPort);
    return 1;
  }

  free(response);
  close(serialPort);
  return 0;
}

int powerOff(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin) {
  int serialPort;
  char *response;
  time_t timePowdCmd, timePwrKey;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    return 1;
  }
  response = malloc(sizeof(char));
  if (0 != querySerialPort(&response, serialPort, "AT+QPOWD", 20)) {
    printf("Error querying serial port\n");
    free(response);
    close(serialPort);
    return 1;
  }
  free(response);
  close(serialPort);

  timePowdCmd = timePwrKey = time(NULL);
  while (checkPinVal(pathToChip, statusPin)) {
    sleep(1);
    if (time(NULL) - timePowdCmd > 24) {
      printf("Could not switch modem off\n");
      return 1;
    } else if (time(NULL) - timePwrKey > 12) {
      printf("Error: Modem status pin did not switch off after 12s\nUsing "
             "power key\n");
      if (0 != modemPowerToggle(pathToChip, pwrKeyPin)) {
        printf("Error toggling modem power\n");
        return 1;
      }
      timePwrKey = time(NULL);
    }
  }

  return 0;
}

int setupPublicConnection(char *pathToPort) {
  unsigned long idx;
  int serialPort;
  char *response = malloc(sizeof(char)),
       *cmdList[] = {"AT+QCFG=\"nwscanmode\",3,1", "AT+QCFG=\"iotopmode\",1,1",
                     "AT+QCFG=\"band\",F,400A0E189F,A0E189F,1",
                     "AT+QCFG=\"servicedomain\",1,1", "AT+COPS=0"};

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  for (idx = 0; idx < sizeof(cmdList) / sizeof(*cmdList); idx++) {
    if (0 != querySerialPort(&response, serialPort, cmdList[idx], 20)) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(response);
      return 1;
    }
    if (NULL == strstr(response, "\r\nOK\r\n")) {
      printf("Module responded not OK. Response:\n%s\n", response);
      close(serialPort);
      free(response);
      return 1;
    }
  }

  close(serialPort);
  free(response);
  return 0;
}

int waitForConnection(char *pathToPort, int waitTimeSeconds) {
  int serialPort, refreshPeriodSeconds = 2;
  char *response = malloc(sizeof(char)), *strPtr;
  time_t startTime;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  startTime = time(NULL);
  do {
    if (0 != querySerialPort(&response, serialPort, "AT+COPS?", 180)) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(response);
      return 1;
    }
    strPtr = strstr(response, "+COPS: ");
    if (waitTimeSeconds - (time(NULL) - startTime) < refreshPeriodSeconds) {
      printf("Connection takes longer than %d seconds to initiate\n",
             waitTimeSeconds);
      close(serialPort);
      free(response);
      return 1;
    }
    sleep(refreshPeriodSeconds);
  } while (!strchr(strPtr, ','));

  close(serialPort);
  free(response);
  return 0;
}

int tcpipInit(char *pathToPort) {
  int serialPort;
  char *response = malloc(sizeof(char)), *responseCpy, *strToken;
  time_t startTime;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+CPIN?", 5)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  }
  if (NULL == strstr(response, "+CPIN: READY")) {
    printf("(U)SIM not ready\nResponse:\n%s\n", response);
    close(serialPort);
    free(response);
    return 1;
  }

  responseCpy = malloc(sizeof(char));
  startTime = time(NULL);
  while (1) {
    if (0 != querySerialPort(&response, serialPort, "AT+CEREG?", 5)) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(responseCpy);
      free(response);
      return 1;
    }
    responseCpy = realloc(responseCpy, sizeof(char) * (strlen(response) + 1));
    strncpy(responseCpy, response, strlen(response) + 1);
    strToken = strtok(response, ",");
    strToken = strtok(NULL, ",");
    if (NULL != strpbrk(strToken, "15") || time(NULL) - startTime > 60)
      break;
    sleep(1);
  }
  free(responseCpy);

  if (0 != querySerialPort(&response, serialPort,
                           "AT+QICSGP=1,1,\"internet\",\"\",\"\",1", 5)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QIACT=1", 150)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  }
  if (NULL == strstr(response, "\r\nOK\r\n")) {
    printf("Failed to activate PDP context\nDeactivating\n");
    if (0 != querySerialPort(&response, serialPort, "AT+QIDEACT=1", 40)) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(response);
      return 1;
    }
  }

  close(serialPort);
  free(response);
  return 0;
}

int tcpipClose(char *pathToPort) {
  int serialPort;
  char *response = malloc(sizeof(char));

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QIDEACT=1", 40)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  }

  return 0;
}
