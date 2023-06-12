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

int main(int argc, char **argv) {
  /* char *cmdList5GTNCfg[] = {"AT+QCFG=\"servicedomain\",1",
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

  if (argc == 1) {
    printf("No arguments provided\n");
    return 1;
  }

  else if (0 == strcmp(argv[1], "--init")) {

    if (0 != powerOn(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                     PIN_NUM_PWR_KEY)) {
      printf("Error powering the module on\n");
      return 1;
    }

    if (0 != cfgPublicNetwork(PATH_TO_SERIAL_PORT)) {
      printf("Error setting public network configuration\n");
      return 1;
    }

    if (0 != waitForNetwork(PATH_TO_SERIAL_PORT, 120)) {
      printf("Error while waiting for network\n");
      return 1;
    }

    return 0;
  }

  else if (0 == strcmp(argv[1], "--stop")) {
    if (0 != mqttDisc(PATH_TO_SERIAL_PORT)) {
      printf("Could not disconnect MQTT\n");
    }
    if (0 != powerOff(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                      PIN_NUM_PWR_KEY)) {
      printf("Error powering the module off\n");
      return 1;
    }
    return 0;
  }

  else if (0 == strcmp(argv[1], "--netinfo-pub")) {
    networkData *nD = malloc(sizeof(networkData));

    initNetworkData(nD);
    gatherData(nD, PATH_TO_SERIAL_PORT);

    printf("Connecting to the MQTT broker\n");
    if (0 != mqttConn(PATH_TO_SERIAL_PORT, "io.adafruit.com", 8883, "kboz",
                      "aio_lvhz87FJ0rIlidH8g30V5mQ7FK7s")) {
      printf("Error connecting MQTT\n");
      return 1;
    }

    /* if (0 != mqttConn(PATH_TO_SERIAL_PORT,
                      "29038f72a9d84f18940e0d44daffa687.s2.eu.hivemq.cloud",
                      8883, "rpi5gtn", "Raspi132.")) {
      printf("Error connecting MQTT\n");
      return 1;
    } */

    printf("Publishing network information\n");
    if (0 != mqttPubNetData(PATH_TO_SERIAL_PORT, *nD)) {
      printf("Error publishing to MQTT server\n");
      return 1;
    }

    printf("Disconnecting from the MQTT broker\n");
    if (0 != mqttDisc(PATH_TO_SERIAL_PORT)) {
      printf("Could not disconnect MQTT\n");
      return 1;
    }

    printf("Done\n");
    freeNetworkData(nD);
    free(nD);
    return 0;
  }

  else {
    printf("Unknown arguments\n");
    return 1;
  }
}
