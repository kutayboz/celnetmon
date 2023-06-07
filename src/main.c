#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gpio.h>
#include <serial.h>

int main() {

  char /* *commandList[] = {"AT",
                         "ATI",
                         "AT+QCFG=\"nwscanmode\"",
                         "AT+QCFG=\"iotopmode\"",
                         "AT+COPS?",
                         "AT+CREG?",
                         "AT+CSQ",
                         "AT+QLTS=1",
                         "AT+QNWINFO",
                         "AT+QCSQ",
                         "AT+QSPN",
                         "AT+CCLK?"}, */
      /* *cmdList5GTNCfg[] = {"AT+QCFG=\"servicedomain\",1",
                           "AT+QCFG=\"iotopmode\",1",
                           "AT+QCFG=\"nwscanmode\",3",
                           "AT+QCFG=\"band\",0,8000000,8000000,1",
                           "AT+QICSGP=1,1,\"5gtnouluiot\",\"\",\"\",0",
                           "AT+CGDCONT=1,\"IP\",\"5gtnouluiot\""}, */
      *cmdList5GTNConnectAndPing[] =
          {"AT+COPS=1,2,\"24427\",9",
           "AT+CGATT=1",
           "AT+QHTTPCFG=\"contextid\",1",
           "AT+QHTTPCFG=\"responseheader\",1",
           "AT+QICSGP=1,1,\"5gtnouluiot\",\"\",\"\",0",
           "AT+QIACT=1",
           "AT+QPING=1,\"https://www.google.fi\"",
           "AT+QPING=1,\"www.google.fi\"",
           "AT+QPING=1,\"8.8.8.8\""},
      serialFilePath[] = "/dev/serial0", *output;

  int serialPort, moduleStatus;
  unsigned long idx;
  time_t pwrKeyTime;

  moduleStatus = checkPinVal(27);
  if (0 > moduleStatus) {
    printf("Error checkPinVal()\n");
    return 1;
  } else if (moduleStatus == 0) {
    printf("Module is off\nPowering on\n");
    if (0 != modemPowerToggle(17)) {
      printf("Error toggling modem power\n");
      return 1;
    }
    pwrKeyTime = time(NULL);
    while (!checkPinVal(27)) {
      sleep(1);
      if (time(NULL) - pwrKeyTime > 10) {
        printf("Modem power on takes longer than 10s\nAborting\n");
        return 1;
      }
    }
    sleep(5); /* Wait for APP RDY*/
  }

  serialPort = openSerialPort(serialFilePath);
  if (serialPort < 0) {
    printf("Error openSerialPort()\n");
    return 1;
  }

  output = malloc(1);
  /* for (idx = 0; idx < sizeof(commandList) / sizeof(*commandList); idx++) {
    if (0 != querySerialPort(&output, serialPort, commandList[idx])) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(output);
      return 1;
    }
    printf("%s\n", output);
  } */

  /* for (idx = 0; idx < sizeof(cmdList5GTNCfg) / sizeof(*cmdList5GTNCfg);
  idx++) { if (0 != querySerialPort(&output, serialPort, cmdList5GTNCfg[idx])) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(output);
      return 1;
    }
    printf("%s\n", output);
  }

  printf("Rebooting\n");
  if (0 != modemPowerToggle(17)) {
    printf("Error toggling modem power\n");
    return 1;
  }
  sleep(5);
  if (0 != modemPowerToggle(17)) {
    printf("Error toggling modem power\n");
    return 1;
  }
  pwrKeyTime = time(NULL);
  while (!checkPinVal(27)) {
    sleep(1);
    if (time(NULL) - pwrKeyTime > 10) {
      printf("Modem power on takes longer than 10s\nAborting\n");
      return 1;
    }
  }
  sleep(5);*/

  for (idx = 0; idx < sizeof(cmdList5GTNConnectAndPing) /
                          sizeof(*cmdList5GTNConnectAndPing);
       idx++) {
    if (0 !=
        querySerialPort(&output, serialPort, cmdList5GTNConnectAndPing[idx])) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(output);
      return 1;
    }
    printf("%s\n", output);
  }

  if (0 != querySerialPort(&output, serialPort, "AT+QIACT=0")) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(output);
    return 1;
  }
  printf("%s\n", output);

  if (0 != querySerialPort(&output, serialPort, "AT+CGATT=0")) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(output);
    return 1;
  }
  printf("%s\n", output);

  close(serialPort);
  free(output);
  return 0;
}
