#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bg96.h"

#define PATH_TO_SERIAL_PORT "/dev/serial0"
#define PATH_TO_GPIO_CHIP "/dev/gpiochip0"
#define PIN_NUM_PWR_KEY 17
#define PIN_NUM_STATUS 27

int main() {
  /* char *commandList[] = {"AT",
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
                         "AT+CCLK?"},
       *cmdList5GTNCfg[] = {"AT+QCFG=\"servicedomain\",1",
                            "AT+QCFG=\"iotopmode\",1",
                            "AT+QCFG=\"nwscanmode\",3",
                            "AT+QCFG=\"band\",0,8000000,8000000,1",
                            "AT+QICSGP=1,1,\"5gtnouluiot\",\"\",\"\",0",
                            "AT+CGDCONT=1,\"IP\",\"5gtnouluiot\""},
       *cmdList5GTNConnectAndPing[] = {
           "AT+COPS=1,2,\"24427\",9",
           "AT+CGATT=1",
           "AT+QHTTPCFG=\"contextid\",1",
           "AT+QHTTPCFG=\"responseheader\",1",
           "AT+QICSGP=1,1,\"5gtnouluiot\",\"\",\"\",0",
           "AT+QIACT=1",
           "AT+QPING=1,\"https://www.google.fi\"",
           "AT+QPING=1,\"www.google.fi\"",
           "AT+QPING=1,\"8.8.8.8\""}; */

  if (0 != powerOn(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                   PIN_NUM_PWR_KEY)) {
    printf("Error powering the module on\n");
    return 1;
  }

  if (0 != setupPublicConnection(PATH_TO_SERIAL_PORT)) {
    printf("Error setting public network configuration\n");
    return 1;
  }

  if (0 != waitForConnection(PATH_TO_SERIAL_PORT, 120)) {
    printf("Error while waiting for connection\n");
    return 1;
  }

  if (0 != tcpipInit(PATH_TO_SERIAL_PORT)) {
    printf("Error initiating TCP/IP context\nPowering off\n");
    if (0 != powerOff(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                      PIN_NUM_PWR_KEY)) {
      printf("Error powering the module off\n");
      return 1;
    }
    return 1;
  }

  if (0 != tcpipClose(PATH_TO_SERIAL_PORT)) {
    printf("Error closing TCP/IP context\nPowering off\n");
    if (0 != powerOff(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                      PIN_NUM_PWR_KEY)) {
      printf("Error powering the module off\n");
      return 1;
    }
    return 1;
  }

  if (0 != powerOff(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                    PIN_NUM_PWR_KEY)) {
    printf("Error powering the module off\n");
    return 1;
  }

  return 0;
}
