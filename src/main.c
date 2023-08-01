#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bg96.h"
#include "utils.h"

struct structCfg dataCfg = {NULL, NULL, -1, -1};

int main(int argc, char **argv) {

  if (argc == 1) {
    printf("No arguments provided.\n");
    return 1;
  }

  if (readCfg(&dataCfg, argv[0]) != 0) {
    printf("Error reading the config file.\n");
    return 1;
  }

  if (0 == strcmp(argv[1], "--initpublic")) {

    printf("Powering on\n");
    if (0 != powerOn(dataCfg.pathSerialPort, dataCfg.pathGpioChip,
                     dataCfg.pinStatus, dataCfg.pinPwrKey)) {
      printf("Error powering the module on\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Initiating GNSS\n");
    if (0 != initGNSS(dataCfg.pathSerialPort)) {
      printf("Error initiating GNSS\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Configuring the network settings for DNA public network\n");
    if (0 != cfgPublicNetwork(dataCfg.pathSerialPort)) {
      printf("Error setting public network configuration\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Waiting for establishing network connection\n");
    if (0 != waitForNetwork(dataCfg.pathSerialPort, 120)) {
      printf("Error while waiting for network\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Initialization complete\n");
    freeCfgData(&dataCfg);
    return 0;
  }

  else if (0 == strcmp(argv[1], "--init6GTN")) {

    printf("Powering on\n");
    if (0 != powerOn(dataCfg.pathSerialPort, dataCfg.pathGpioChip,
                     dataCfg.pinStatus, dataCfg.pinPwrKey)) {
      printf("Error powering the module on\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Initiating GNSS\n");
    if (0 != initGNSS(dataCfg.pathSerialPort)) {
      printf("Error initiating GNSS\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Configuring the network settings for 6GTN\n");
    if (0 != cfg6GTN(dataCfg.pathSerialPort)) {
      printf("Error setting 6GTN network configuration\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Waiting for establishing network connection\n");
    if (0 != connect6GTN(dataCfg.pathSerialPort, 120)) {
      printf("Error while connecting to 6GTN\n");
      freeCfgData(&dataCfg);
      return 1;
    }

    printf("Initialization complete\n");
    freeCfgData(&dataCfg);
    return 0;
  }

  else if (0 == strcmp(argv[1], "--stop")) {

    printf("Stopping GNSS\n");
    if (0 != stopGNSS(dataCfg.pathSerialPort)) {
      printf("Error stopping GNSS\n");
    }

    printf("Disconnecting MQTT\n");
    if (0 != mqttDisc(dataCfg.pathSerialPort)) {
      printf("Could not disconnect MQTT\n");
    }

    printf("Powering off\n");
    if (0 != powerOff(dataCfg.pathSerialPort, dataCfg.pathGpioChip,
                      dataCfg.pinStatus, dataCfg.pinPwrKey)) {
      printf("Error powering the module off\n");
      freeCfgData(&dataCfg);
      return 1;
    }
    printf("Power off complete\n");
    freeCfgData(&dataCfg);
    return 0;
  }

  else if (0 == strcmp(argv[1], "--netinfo-pub")) {
    networkData *nD = malloc(sizeof(networkData));

    printf("Obtaining network info\n");
    initNetworkData(nD);
    if (gatherData(nD, dataCfg.pathSerialPort) != 0) {
      printf("Failed to gather network info\n");
      goto netinfopubFail;
    }

    printf("Connecting to the MQTT broker\n");
    if (0 != mqttConn(dataCfg.pathSerialPort, "io.adafruit.com", 8883, "kboz",
                      "aio_lvhz87FJ0rIlidH8g30V5mQ7FK7s")) {
      printf("Error connecting MQTT\n");
      goto netinfopubFail;
    }

    printf("Publishing network information\n");
    if (0 != mqttPubNetData(dataCfg.pathSerialPort, *nD)) {
      printf("Error publishing to MQTT server\n");
      goto netinfopubFail;
    }

    printf("Disconnecting from the MQTT broker\n");
    if (0 != mqttDisc(dataCfg.pathSerialPort)) {
      printf("Could not disconnect MQTT\n");
      goto netinfopubFail;
    }

    printf("Network info publish complete\n");
    freeNetworkData(nD);
    free(nD);
    freeCfgData(&dataCfg);
    return 0;

  netinfopubFail:
    freeNetworkData(nD);
    free(nD);
    freeCfgData(&dataCfg);
    return 1;
  }

  else {
    printf("Unknown arguments\n");
    freeCfgData(&dataCfg);
    return 1;
  }
}
