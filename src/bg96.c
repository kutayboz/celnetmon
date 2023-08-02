#include "bg96.h"
#include "serial.h"
#include "utils.h"
#include <gpiod.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int modemPowerToggle(char *pathToChip, int pinNum) {
  struct gpiod_chip *chip;
  struct gpiod_line *line;

  chip = gpiod_chip_open(pathToChip);
  if (!chip) {
    perror("Error opening GPIO chip");
    return 1;
  }

  line = gpiod_chip_get_line(chip, pinNum);
  if (!line) {
    perror("Error getting GPIO line");
    gpiod_chip_close(chip);
    return 1;
  }

  if (gpiod_line_request_output(line, "cellular modem power toggle", 0) < 0) {
    perror("Error setting GPIO line as output");
    gpiod_chip_close(chip);
    return 1;
  }

  if (gpiod_line_set_value(line, 1) < 0) {
    perror("Error setting GPIO line high");
    gpiod_chip_close(chip);
    return 1;
  }

  sleep(1);

  if (gpiod_line_set_value(line, 0) < 0) {
    perror("Error setting GPIO line low");
    gpiod_chip_close(chip);
    return 1;
  }

  gpiod_chip_close(chip);
  return 0;
}

int initNetworkData(networkData *nD) {
  nD->operatorName = calloc(1, sizeof(char));
  nD->networkName = calloc(1, sizeof(char));
  nD->tech = calloc(1, sizeof(char));
  nD->band = calloc(1, sizeof(char));
  nD->channel = calloc(1, sizeof(char));
  nD->networkRegStat = calloc(1, sizeof(char));
  nD->trkAreaCode = calloc(1, sizeof(char));
  nD->cellID = calloc(1, sizeof(char));
  nD->rssi = calloc(1, sizeof(char));
  nD->rsrp = calloc(1, sizeof(char));
  nD->sinr = calloc(1, sizeof(char));
  nD->rsrq = calloc(1, sizeof(char));
  nD->ber = calloc(1, sizeof(char));
  nD->lat = calloc(1, sizeof(char));
  nD->longt = calloc(1, sizeof(char));
  nD->alt = calloc(1, sizeof(char));
  nD->gpsTime = calloc(1, sizeof(char));
  return 0;
}

int freeNetworkData(networkData *nD) {
  free(nD->operatorName);
  free(nD->networkName);
  free(nD->tech);
  free(nD->band);
  free(nD->channel);
  free(nD->networkRegStat);
  free(nD->trkAreaCode);
  free(nD->cellID);
  free(nD->rssi);
  free(nD->rsrp);
  free(nD->sinr);
  free(nD->rsrq);
  free(nD->ber);
  free(nD->lat);
  free(nD->longt);
  free(nD->alt);
  free(nD->gpsTime);
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

int configureNetwork(char *pathToPort) {
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

int cfg6GTN(char *pathToPort) {
  unsigned long idx;
  int serialPort;
  char *response = malloc(sizeof(char)),
       *cmdList[] = {"AT+QCFG=\"nwscanmode\",3,1",
                     "AT+QCFG=\"iotopmode\",1,1",
                     "AT+QCFG=\"band\",0,8000000,8000000,1",
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
                           "AT+QICSGP=1,1,\"5gtnouluiot\",\"\",\"\",0", 5,
                           NULL)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  }

  /* Test the function without below commented block. */
  /* if (0 != querySerialPort(&response, serialPort,
                           "AT+CGDCONT=1,\"IP\",\"5gtnouluiot\"", 2, NULL)) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(response);
    return 1;
  } */

  close(serialPort);
  free(response);
  return 0;
}

int defineNetworkDetails(char *pathToPort/* , int waitTimeSeconds */) {
  int serialPort/* , refreshPeriodSeconds = 1 */;
  char *response = malloc(sizeof(char))/* , *responseCpy, *strPtr */;
  /* time_t startTime; */

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+COPS=1,2,\"24427\",9",
                           180, NULL)) {
    printf("Error querying AT+COPS=1\n");
    close(serialPort);
    free(response);
    return 1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+CGATT=1", 140, NULL)) {
    printf("Error querying AT+CGATT=1\n");
    close(serialPort);
    free(response);
    return 1;
  }

  /* startTime = time(NULL);
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
  free(responseCpy);*/

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

int mqttPubNetData(char *pathToPort, networkData nD) {
  int serialPort, cmdStrLen, msgID, msgSize;
  char *response = malloc(sizeof(char)), *command = malloc(sizeof(char)),
       *msg = malloc(sizeof(char)),
       textDataTemplate[] = "Operator: %s\n"
                            "Network: %s\n"
                            "Reg Status: %s\n"
                            "Tracking Area Code: %s\n"
                            "Cell ID: %s\n"
                            "Technology: %s\n"
                            "Band: %s\n"
                            "Channel: %s\n"
                            "Network Time (GMT): %s",
       jsonTemplate[] = "{\n"
                        "\t\"value\": %s,\n"
                        "\t\"lat\": %s,\n"
                        "\t\"lon\": %s,\n"
                        "\t\"ele\": %s\n"
                        "}";

  msgSize = sizeof(char) * (strlen(textDataTemplate) + strlen(nD.operatorName) +
                            strlen(nD.networkName) + strlen(nD.networkRegStat) +
                            strlen(nD.trkAreaCode) + strlen(nD.cellID) +
                            strlen(nD.tech) + strlen(nD.band) +
                            strlen(nD.channel) + strlen(asctime(&nD.netTime)) +
                            1 - 9 * 2); /* REMOVE THE EXTRA %s FROM TEMPLATE */
  msg = realloc(msg, msgSize);
  snprintf(msg, msgSize / sizeof(char), textDataTemplate, nD.operatorName,
           nD.networkName, nD.networkRegStat, nD.trkAreaCode, nD.cellID,
           nD.tech, nD.band, nD.channel, asctime(&nD.netTime));

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto mqttPubError2;
  }

  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/textinfo") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/textinfo", msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  msgSize = sizeof(char) * (strlen(nD.rssi) + 1);
  msg = realloc(msg, msgSize);
  strncpy(msg, nD.rssi, msgSize / sizeof(char));
  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/rssi") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/rssi", msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  msgSize = sizeof(char) * (strlen(nD.rsrp) + 1);
  msg = realloc(msg, msgSize);
  strncpy(msg, nD.rsrp, msgSize / sizeof(char));
  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/rsrp") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/rsrp", msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  msgSize = sizeof(char) * (strlen(nD.sinr) + 1);
  msg = realloc(msg, msgSize);
  strncpy(msg, nD.sinr, msgSize / sizeof(char));
  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/sinr") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/sinr", msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  msgSize = sizeof(char) * (strlen(nD.rsrq) + 1);
  msg = realloc(msg, msgSize);
  strncpy(msg, nD.rsrq, msgSize / sizeof(char));
  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/rsrq") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/rsrq", msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  msgSize = sizeof(char) * (strlen(nD.ber) + 1);
  msg = realloc(msg, msgSize);
  strncpy(msg, nD.ber, msgSize / sizeof(char));
  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/ber") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/ber", msgSize);
  if (0 != querySerialPort(&response, serialPort, command, 15, (char *)-1)) {
    printf("Error querying publish command: %s\n", command);
    goto mqttPubError1;
  } else if (0 !=
             querySerialPort(&response, serialPort, msg, 15, "\r\n+QMTPUB:")) {
    printf("Error querying publish message\n");
    goto mqttPubError1;
  }

  msgSize =
      sizeof(char) * (strlen(jsonTemplate) + strlen(nD.lat) + strlen(nD.longt) +
                      strlen(nD.alt) + strlen(nD.gpsTime) + 1 -
                      4 * 2); /* REMOVE THE EXTRA %s FROM TEMPLATE */
  msg = realloc(msg, msgSize);
  snprintf(msg, msgSize / sizeof(char), jsonTemplate, nD.gpsTime, nD.lat,
           nD.longt, nD.alt);

  msgID = rand() % 65536;
  cmdStrLen = strlen("AT+QMTPUB=0") + (int)ceil(log10(msgID)) + 1 + 1 +
              strlen("kboz/feeds/gpsdata/json") + (int)ceil(log10(msgSize)) + 8;
  command = realloc(command, sizeof(char) * (cmdStrLen + 1));
  snprintf(command, cmdStrLen + 1, "AT+QMTPUB=0,%d,1,0,\"%s\",%d", msgID,
           "kboz/feeds/gpsdata/json", msgSize);
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
  free(msg);
  close(serialPort);
  return 0;

mqttPubError1:
  close(serialPort);
mqttPubError2:
  free(command);
  free(response);
  free(msg);
  return 1;
}

int gatherData(networkData *output, char *pathToPort) {
  int serialPort, strLen, tmpNum;
  char *response = malloc(sizeof(char)), *varPtr, *qcsqSystemMode;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto gatherDataError2;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+COPS?", 1, NULL)) {
    printf("Error querying AT+COPS?\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, "\",\r");
  while (strstr(varPtr, "\n+COPS:") == NULL) {
    varPtr = strtok(NULL, "\",\r");
  }
  varPtr = strtok(NULL, "\",\r");
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->operatorName = realloc(output->operatorName, sizeof(char) * strLen);
  strncpy(output->operatorName, varPtr, strLen);

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
  varPtr = strtok(NULL, " ,\r");
  if (0 == strcmp(varPtr, "99")) {
    strLen = strlen("NULL") + 1;
    output->ber = realloc(output->ber, sizeof(char) * strLen);
    strncpy(output->ber, "NULL", strLen);
  } else {
    tmpNum = exp2(strtoimax(varPtr, NULL, 10));
    strLen = 7;
    output->ber = realloc(output->ber, sizeof(char) * strLen);
    snprintf(output->ber, strLen * sizeof(char), "%f.2%%", 0.14 * tmpNum);
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QLTS=1", 1, NULL)) {
    printf("Error querying AT+QLTS=1\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, " ,\r");
  while (strstr(varPtr, "\n+QLTS:") == NULL) {
    varPtr = strtok(NULL, " ,\r");
  }
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_year = strtoimax(varPtr, NULL, 10) - 1900;
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_mon = strtoimax(varPtr, NULL, 10) - 1;
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_mday = strtoimax(varPtr, NULL, 10);
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_hour = strtoimax(varPtr, NULL, 10);
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_min = strtoimax(varPtr, NULL, 10);
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_sec = strtoimax(varPtr, NULL, 10);
  varPtr = strtok(NULL, " \"/:+-,\r");
  varPtr = strtok(NULL, " \"/:+-,\r");
  output->netTime.tm_isdst = strtoimax(varPtr, NULL, 2);
  output->netTime.tm_wday = 0;
  output->netTime.tm_yday = 0;
  output->netTime.tm_gmtoff = 0;
  if (-1 == mktime(&output->netTime)) {
    printf("Error parsing network time\n");
    goto gatherDataError1;
  }
  output->netTime.tm_gmtoff = 0;
  output->netTime.tm_zone = NULL;

  if (0 != querySerialPort(&response, serialPort, "AT+QNWINFO", 1, NULL)) {
    printf("Error querying AT+QNWINFO\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, "\",\r");
  while (strstr(varPtr, "\n+QNWINFO:") == NULL) {
    varPtr = strtok(NULL, " \",\r");
  }
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->tech = realloc(output->tech, sizeof(char) * strLen);
  strncpy(output->tech, varPtr, strLen);
  varPtr = strtok(NULL, "\",\r");
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->band = realloc(output->band, sizeof(char) * strLen);
  strncpy(output->band, varPtr, strLen);
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->channel = realloc(output->channel, sizeof(char) * strLen);
  strncpy(output->channel, varPtr, strLen);

  if (0 != querySerialPort(&response, serialPort, "AT+QSPN", 1, NULL)) {
    printf("Error querying AT+QSPN\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, "\",\r");
  while (strstr(varPtr, "\n+QSPN:") == NULL) {
    varPtr = strtok(NULL, " \",\r");
  }
  varPtr = strtok(NULL, "\",\r");
  strLen = strlen(varPtr) + 1;
  output->networkName = realloc(output->networkName, sizeof(char) * strLen);
  strncpy(output->networkName, varPtr, strLen);

  if (0 != querySerialPort(&response, serialPort, "AT+QCSQ", 1, NULL)) {
    printf("Error querying AT+QCSQ\n");
    goto gatherDataError1;
  }
  varPtr = strtok(response, " \",\r");
  while (strstr(varPtr, "\n+QCSQ:") == NULL) {
    varPtr = strtok(NULL, " \",\r");
  }
  qcsqSystemMode = varPtr = strtok(NULL, " \",\r");
  if (0 != strcmp(qcsqSystemMode, "NOSERVICE")) {
    varPtr = strtok(NULL, " \",\r");
    strLen = strlen(varPtr) + 1;
    output->rssi = realloc(output->rssi, sizeof(char) * strLen);
    strncpy(output->rssi, varPtr, strLen);
    if (0 != strcmp(qcsqSystemMode, "GSM")) {
      varPtr = strtok(NULL, " \",\r");
      strLen = strlen(varPtr) + 1;
      output->rsrp = realloc(output->rsrp, sizeof(char) * strLen);
      strncpy(output->rsrp, varPtr, strLen);
      varPtr = strtok(NULL, " \",\r");
      strLen = 7;
      output->sinr = realloc(output->sinr, sizeof(char) * strLen);
      snprintf(output->sinr, 7, "%3.2f", (strtof(varPtr, NULL) - 100) / 5);
      varPtr = strtok(NULL, " \",\r");
      strLen = strlen(varPtr) + 1;
      output->rsrq = realloc(output->rsrq, sizeof(char) * strLen);
      strncpy(output->rsrq, varPtr, strLen);
    } else {
      strLen = strlen("NULL") + 1;
      output->rsrp = realloc(output->rsrp, sizeof(char) * strLen);
      output->sinr = realloc(output->sinr, sizeof(char) * strLen);
      output->rsrq = realloc(output->rsrq, sizeof(char) * strLen);
      strncpy(output->rsrp, "NULL", strLen);
      strncpy(output->sinr, "NULL", strLen);
      strncpy(output->rsrq, "NULL", strLen);
    }
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QGPSLOC=2", 5, NULL)) {
    printf("Error querying AT+QGPSLOC. Response:\n%s\n", response);
    strLen = strlen("NULL") + 1;
    output->lat = realloc(output->lat, sizeof(char) * strLen);
    output->longt = realloc(output->longt, sizeof(char) * strLen);
    output->alt = realloc(output->alt, sizeof(char) * strLen);
    output->gpsTime = realloc(output->gpsTime, sizeof(char) * strLen);
    strncpy(output->lat, "NULL", strLen);
    strncpy(output->longt, "NULL", strLen);
    strncpy(output->alt, "NULL", strLen);
    strncpy(output->gpsTime, "NULL", strLen);
  } else {
    varPtr = strtok(response, " \",\r");
    while (strstr(varPtr, "\n+QGPSLOC:") == NULL) {
      varPtr = strtok(NULL, " \",\r");
    }
    varPtr = strtok(NULL, " \",\r");
    varPtr = strtok(NULL, " \",\r");
    strLen = strlen(varPtr) + 1;
    output->lat = realloc(output->lat, sizeof(char) * strLen);
    strncpy(output->lat, varPtr, strLen);
    varPtr = strtok(NULL, " \",\r");
    strLen = strlen(varPtr) + 1;
    output->longt = realloc(output->longt, sizeof(char) * strLen);
    strncpy(output->longt, varPtr, strLen);
    varPtr = strtok(NULL, " \",\r");
    varPtr = strtok(NULL, " \",\r");
    strLen = strlen(varPtr) + 1;
    output->alt = realloc(output->alt, sizeof(char) * strLen);
    strncpy(output->alt, varPtr, strLen);
    varPtr = strtok(NULL, " \",\r");
    varPtr = strtok(NULL, " \",\r");
    varPtr = strtok(NULL, " \",\r");
    varPtr = strtok(NULL, " \",\r");
    varPtr = strtok(NULL, " \",\r");
    strLen = strlen(varPtr) + 1;
    output->gpsTime = realloc(output->gpsTime, sizeof(char) * strLen);
    strncpy(output->gpsTime, varPtr, strLen);
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

int initGNSS(char *pathToPort) {
  char *response = malloc(sizeof(char));
  int serialPort;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto initGNSSError2;
  }

  if (0 != querySerialPort(&response, serialPort,
                           "AT+QGPSCFG=\"outport\",\"none\"", 1, NULL)) {
    printf("Error querying AT+QGPSCFG\n");
    goto initGNSSError1;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QGPSCFG=\"gnssconfig\",1",
                           1, NULL)) {
    printf("Error querying AT+QGPSCFG\n");
    goto initGNSSError1;
  }

  if (0 !=
      querySerialPort(&response, serialPort, "AT+QGPS=1,30,50,0,1", 5, NULL)) {
    printf("Error querying AT+QGPS\n");
    goto initGNSSError1;
  }

  close(serialPort);
  free(response);
  return 0;

initGNSSError1:
  close(serialPort);
initGNSSError2:
  free(response);
  return 1;
}

int stopGNSS(char *pathToPort) {
  char *response = malloc(sizeof(char));
  int serialPort;

  serialPort = openSerialPort(pathToPort);
  if (0 > serialPort) {
    printf("Error opening serial port %s\n", pathToPort);
    goto initGNSSError2;
  }

  if (0 != querySerialPort(&response, serialPort, "AT+QGPSEND", 1, NULL)) {
    printf("Error querying AT+QGPSEND\n");
    goto initGNSSError1;
  }

  close(serialPort);
  free(response);
  return 0;

initGNSSError1:
  close(serialPort);
initGNSSError2:
  free(response);
  return 1;
}
