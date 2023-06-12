#include "bg96.h"
#include "gpio.h"
#include "serial.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int initNetworkData(struct networkData *nD) {
  nD->operatorName = malloc(sizeof(char));
  nD->currentTech = malloc(sizeof(char));
  nD->networkRegStat = malloc(sizeof(char));
  nD->trkAreaCode = malloc(sizeof(char));
  nD->cellID = malloc(sizeof(char));
  nD->rssi = malloc(sizeof(char));
  nD->ber = malloc(sizeof(char));
  return 0;
}

int freeNetworkData(struct networkData *nD) {
  free(nD->operatorName);
  free(nD->currentTech);
  free(nD->networkRegStat);
  free(nD->trkAreaCode);
  free(nD->cellID);
  free(nD->rssi);
  free(nD->ber);
  return 0;
}

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
  if (0 != querySerialPort(&response, serialPort, "AT", 10, NULL)) {
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
  int serialPort, moduleStatus;
  char *response;
  time_t timePowdCmd, timePwrKey;

  moduleStatus = checkPinVal(pathToChip, statusPin);
  if (0 > moduleStatus) {
    printf("Error checkPinVal()\n");
    return 1;
  } else if (moduleStatus == 0) {
    printf("Module is already off\n");
    return 1;
  }

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    return 1;
  }
  response = malloc(sizeof(char));
  if (0 !=
      querySerialPort(&response, serialPort, "AT+QPOWD", 20, "POWERED DOWN")) {
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

int cfgPublicNetwork(char *pathToPort) {
  unsigned long idx;
  int serialPort;
  char *response = malloc(sizeof(char)),
       *cmdList[] = {"AT+QCFG=\"nwscanmode\",3,1",
                     "AT+QCFG=\"iotopmode\",1,1",
                     "AT+QCFG=\"band\",F,400A0E189F,A0E189F,1",
                     "AT+QCFG=\"servicedomain\",1,1",
                     "AT+COPS=0,0",
                     "AT+CEREG=2"};

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  for (idx = 0; idx < sizeof(cmdList) / sizeof(*cmdList); idx++) {
    if (0 != querySerialPort(&response, serialPort, cmdList[idx], 20, NULL)) {
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

  if (0 != querySerialPort(&response, serialPort, "AT+CPIN?", 2, NULL)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  } else if (NULL == strstr(response, "+CPIN: READY")) {
    printf("(U)SIM not ready\nResponse:\n%s\n", response);
    close(serialPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort,
                           "AT+QICSGP=1,1,\"internet\",\"\",\"\",1", 5, NULL)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  }

  close(serialPort);
  free(response);
  return 0;
}

int waitForNetwork(char *pathToPort, int waitTimeSeconds) {
  int serialPort, refreshPeriodSeconds = 1;
  char *response = malloc(sizeof(char)), *responseCpy, *strPtr;
  time_t startTime;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  startTime = time(NULL);
  do {
    if (0 != querySerialPort(&response, serialPort, "AT+COPS?", 180, NULL)) {
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

  responseCpy = malloc(sizeof(char));
  startTime = time(NULL);
  while (1) {
    if (0 != querySerialPort(&response, serialPort, "AT+CEREG?", 2, NULL)) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(responseCpy);
      free(response);
      return 1;
    }
    responseCpy = realloc(responseCpy, sizeof(char) * (strlen(response) + 1));
    strncpy(responseCpy, response, strlen(response) + 1);
    strPtr = strtok(response, ",");
    strPtr = strtok(NULL, ",");
    if (NULL != strpbrk(strPtr, "15") || time(NULL) - startTime > 60)
      break;
    sleep(1);
  }
  free(responseCpy);

  close(serialPort);
  free(response);
  return 0;
}

int mqttDisc(char *pathToPort) {
  int serialPort;
  char *response = malloc(sizeof(char));

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QMTCONN?", 2, NULL)) {
    printf("Error querying AT+QMTCONN?\n");
    close(serialPort);
    free(response);
    return 1;
  }
  if (NULL != strstr(response, "\r\n+QMTCONN: 0,")) {
    if (0 != querySerialPort(&response, serialPort, "AT+QMTDISC=0", 60,
                             "+QMTDISC:")) {
      printf("Error querying AT+QMTDISC=0\n");
      close(serialPort);
      free(response);
      return 1;
    }
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QMTOPEN?", 2, NULL)) {
    printf("Error querying AT+QMTOPEN?\n");
    close(serialPort);
    free(response);
    return 1;
  }
  if (NULL != strstr(response, "\r\n+QMTOPEN: 0,")) {
    if (0 != querySerialPort(&response, serialPort, "AT+QMTCLOSE=0", 2,
                             "+QMTCLOSE:")) {
      printf("Error querying AT+QMTCLOSE=0\n");
      close(serialPort);
      free(response);
      return 1;
    }
  }

  close(serialPort);
  free(response);
  return 0;
}

int mqttConn(char *pathToPort, char *hostURL, int hostPort, char *username,
             char *password) {
  int serialPort, cmdStrLen;
  unsigned long idx;
  char *response = malloc(sizeof(char)), *command, clientID[11];

  srand(time(NULL));
  for (idx = 0; idx < sizeof(clientID) - 1; idx++) {
    clientID[idx] = '0' + rand() % 10;
  }
  clientID[sizeof(clientID) - 1] = '\0';

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto mqttConnError3;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QMTCFG=\"version\",0,4",
                           2, NULL)) {
    printf("Error querying AT+QMTCFG=\"version\",0,4\n");
    goto mqttConnError2;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QMTCFG=\"pdpcid\",0,1", 2,
                           NULL)) {
    printf("Error querying AT+QMTCFG=\"pdpcid\",0,1\n");
    goto mqttConnError2;
  }

  if (0 != querySerialPort(&response, serialPort,
                           "AT+QMTCFG=\"timeout\",0,5,3,0", 2, NULL)) {
    printf("Error configuring MQTT package timeout\n");
    goto mqttConnError2;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QMTCFG=\"ssl\",0,1,0", 2,
                           NULL)) {
    printf("Error querying AT+QMTCFG=\"ssl\",0,1,0\n");
    goto mqttConnError2;
  }

  cmdStrLen = strlen("AT+QMTOPEN=0") + strlen(hostURL) +
              (int)(1 + ceil(log10(hostPort))) + 5;
  command = malloc(sizeof(char) * (cmdStrLen));
  snprintf(command, cmdStrLen - 1, "AT+QMTOPEN=0,\"%s\",%d", hostURL, hostPort);
  if (0 != querySerialPort(&response, serialPort, command, 75, "+QMTOPEN:")) {
    printf("Error querying %s\n", command);
    goto mqttConnError1;
  } else if (NULL == strstr(response, "+QMTOPEN: 0,0")) {
    printf("Failed to establish network for MQTT connection\n");
    goto mqttConnError1;
  }

  if (username == NULL) {
    cmdStrLen = strlen("AT+QMTCONN=0") + strlen(clientID) + 9;
    command = realloc(command, sizeof(char) * (cmdStrLen + 1));
    snprintf(command, cmdStrLen + 1, "AT+QMTCONN=0,\"%s\"", clientID);
  } else if (password == NULL) {
    cmdStrLen =
        strlen("AT+QMTCONN=0") + strlen(clientID) + strlen(username) + 9;
    command = realloc(command, sizeof(char) * (cmdStrLen + 1));
    snprintf(command, cmdStrLen + 1, "AT+QMTCONN=0,\"%s\",\"%s\"", clientID,
             username);
  } else {
    cmdStrLen = strlen("AT+QMTCONN=0") + strlen(clientID) + strlen(username) +
                strlen(password) + 9;
    command = realloc(command, sizeof(char) * (cmdStrLen + 1));
    snprintf(command, cmdStrLen + 1, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",
             clientID, username, password);
  }
  if (0 != querySerialPort(&response, serialPort, command, 70, "\r\n+QMT")) {
    printf("Failed to query %s", command);
    goto mqttConnError1;
  } else if (NULL == strstr(response, "\r\nOK\r\n")) {
    printf("Error with command. Response:\n%s\n", response);
    goto mqttConnError1;
  } else if (NULL != strstr(response, "+QMTSTAT:")) {
    printf("Dropped from network. Response:\n%s\n", response);
    goto mqttConnError1;
  } else if (NULL == strstr(response, "+QMTCONN: 0,0")) {
    printf("Failed to establish connection. Response:\n%s\n", response);
    goto mqttConnError1;
  }

  free(command);
  close(serialPort);
  free(response);
  return 0;

mqttConnError1:
  free(command);
mqttConnError2:
  close(serialPort);
mqttConnError3:
  free(response);
  return 1;
}

int mqttPub(char *pathToPort, char *topic, char *msg) {
  int serialPort, cmdStrLen, msgID, msgSize;
  char *response = malloc(sizeof(char)), *command;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto mqttPubError2;
  }

  msgSize = sizeof(char) * strlen(msg);
  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen(topic) + (int)ceil(log10(msgSize)) + 8;
  command = malloc(sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID, topic,
           msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  free(command);
  free(response);
  close(serialPort);
  return 0;

mqttPubError1:
  free(command);
  free(response);
mqttPubError2:
  close(serialPort);
  return 1;
}

int gatherData(struct networkData *output, char *pathToPort) {
  int serialPort, strLen, tmpNum;
  char *response = malloc(sizeof(char)), *varPtr;
  /* char *commandList[] = {"AT+COPS?",
                         "AT+CEREG?",
                         "AT+CSQ",
                         "AT+QLTS=1",
                         "AT+QNWINFO",
                         "AT+QCSQ",
                         "AT+QSPN",
                         "AT+CCLK?"}; */

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto gatherDataError2;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+COPS?", 1, NULL)) {
    printf("Error querying AT+COPS?\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, ",\r");
  while (strstr(varPtr, "\n+COPS:") == NULL) {
    varPtr = strtok(NULL, ",\r");
  }
  varPtr = strtok(NULL, ",\r");
  varPtr = strtok(NULL, ",\r");
  strLen = strlen(varPtr) + 1;
  output->operatorName = realloc(output->operatorName, sizeof(char) * strLen);
  strncpy(output->operatorName, varPtr, strLen);
  varPtr = strtok(NULL, "\",\r");
  switch (*varPtr) {
  case '0':
    strLen = strlen("GSM") + 1;
    output->currentTech = realloc(output->currentTech, sizeof(char) * strLen);
    strncpy(output->currentTech, "GSM", strLen);
    break;

  case '8':
    strLen = strlen("LTE Cat M1") + 1;
    output->currentTech = realloc(output->currentTech, sizeof(char) * strLen);
    strncpy(output->currentTech, "LTE Cat M1", strLen);
    break;

  case '9':
    strLen = strlen("LTE Cat NB1") + 1;
    output->currentTech = realloc(output->currentTech, sizeof(char) * strLen);
    strncpy(output->currentTech, "LTE Cat NB1", strLen);
    break;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+CEREG?", 1, NULL)) {
    printf("Error querying AT+CEREG?\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, ",\r");
  while (strstr(varPtr, "\n+CEREG:") == NULL) {
    varPtr = strtok(NULL, ",\r");
  }
  varPtr = strtok(NULL, ",\r");
  switch (*varPtr) {
  case '1':
    strLen = strlen("Not roaming") + 1;
    output->networkRegStat =
        realloc(output->networkRegStat, sizeof(char) * strLen);
    strncpy(output->networkRegStat, "Not roaming", strLen);
    break;

  case '5':
    strLen = strlen("Roaming") + 1;
    output->networkRegStat =
        realloc(output->networkRegStat, sizeof(char) * strLen);
    strncpy(output->networkRegStat, "Roaming", strLen);
    break;
  }
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->trkAreaCode = realloc(output->trkAreaCode, sizeof(char) * strLen);
  strncpy(output->trkAreaCode, varPtr, strLen);
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->cellID = realloc(output->cellID, sizeof(char) * strLen);
  strncpy(output->cellID, varPtr, strLen);

  if (0 != querySerialPort(&response, serialPort, "AT+CSQ", 1, NULL)) {
    printf("Error querying AT+CSQ\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, ",\r");
  while (strstr(varPtr, "\n+CSQ:") == NULL) {
    varPtr = strtok(NULL, " ,\r");
  }
  varPtr = strtok(NULL, " ,\r");
  if (0 == strcmp(varPtr, "99")) {
    strLen = strlen("unknown") + 1;
    output->rssi = realloc(output->rssi, sizeof(char) * strLen);
    strncpy(output->rssi, "unknown", strLen);
  } else {
    tmpNum = (2 * strtoimax(varPtr, NULL, 10) - 113);
    strLen = 5;
    output->rssi = realloc(output->rssi, sizeof(char) * strLen);
    snprintf(output->rssi, strLen * sizeof(char), "%d", tmpNum);
  }
  varPtr = strtok(NULL, " ,\r");
  if (0 == strcmp(varPtr, "99")) {
    strLen = strlen("unknown") + 1;
    output->ber = realloc(output->ber, sizeof(char) * strLen);
    strncpy(output->ber, "unknown", strLen);
  } else {
    tmpNum = exp2(strtoimax(varPtr, NULL, 10));
    strLen = 7;
    output->ber = realloc(output->ber, sizeof(char) * strLen);
    snprintf(output->ber, strLen * sizeof(char), "%f.2%%", 0.14 * tmpNum);
  }

  close(serialPort);
  free(response);
  return 0;

gatherDataError1:
  close(serialPort);
gatherDataError2:
  free(response);
  return 1;
}
