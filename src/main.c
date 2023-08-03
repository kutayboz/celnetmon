#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bg96.h"
#include "utils.h"

listCfgData cfgData = {NULL, NULL};

int main(int argc, char **argv) {

  if (argc == 1) {
    printf("No arguments provided.\n");
    return 1;
  }

  if (readCfg(&cfgData, argv[0]) != 0) {
    printf("Error reading the config file.\n");
    return 1;
  }

  /* if (0 == strcmp(argv[1], "--initpublic")) {

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
    if (0 != configureNetwork(dataCfg.pathSerialPort)) {
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
  } */

  if (0 == strcmp(argv[1], "--init")) {

    printf("Powering on\n");
    if (0 != powerOn(cfgData)) {
      printf("Error powering the module on\n");
      freeCfgData(&cfgData);
      return 1;
    }

    printf("Initiating GNSS\n");
    if (0 != initGNSS(cfgData)) {
      printf("Error initiating GNSS\n");
      freeCfgData(&cfgData);
      return 1;
    }

    printf("Configuring the network settings\n");
    if (0 != configureNetwork(cfgData)) {
      printf("Error setting network configuration\n");
      freeCfgData(&cfgData);
      return 1;
    }

    if (cfgData.intDataList[NETWORK_OPERATOR_POS].value != -1) {
      printf("Defining operator \"%d\"\n",
             cfgData.intDataList[NETWORK_OPERATOR_POS].value);
      if (0 != defineNetworkDetails(cfgData)) {
        printf("Error defining the specified operator\n");
        freeCfgData(&cfgData);
        return 1;
      }
    }

    printf("Waiting for establishing network connection\n");
    if (0 != waitForNetwork(cfgData, 120)) {
      printf("Error while waiting for network connection\n");
      freeCfgData(&cfgData);
      return 1;
    }

    printf("Initialization complete\n");
    freeCfgData(&cfgData);
    return 0;
  }

  else if (0 == strcmp(argv[1], "--stop")) {

    printf("Stopping GNSS\n");
    if (0 != stopGNSS(cfgData)) {
      printf("Error stopping GNSS\n");
    }

    printf("Disconnecting MQTT\n");
    if (0 != mqttDisc(cfgData)) {
      printf("Could not disconnect MQTT\n");
    }

    printf("Powering off\n");
    if (0 != powerOff(cfgData)) {
      printf("Error powering the module off\n");
      freeCfgData(&cfgData);
      return 1;
    }
    printf("Power off complete\n");
    freeCfgData(&cfgData);
    return 0;
  }

  else if (0 == strcmp(argv[1], "--publish")) {
    networkData *nD = malloc(sizeof(networkData));

    printf("Obtaining network info\n");
    initNetworkData(nD);
    if (gatherData(nD, cfgData) != 0) {
      printf("Failed to gather network info\n");
      goto netinfopubFail;
    }

    printf("Connecting to the MQTT broker\n");
    if (0 != mqttConn(cfgData)) {
      printf("Error connecting MQTT\n");
      goto netinfopubFail;
    }

    printf("Publishing network information\n");
    if (0 != mqttPubNetData(cfgData, *nD)) {
      printf("Error publishing to MQTT server\n");
      goto netinfopubFail;
    }

    printf("Disconnecting from the MQTT broker\n");
    if (0 != mqttDisc(cfgData)) {
      printf("Could not disconnect MQTT\n");
      goto netinfopubFail;
    }

    printf("Network info publish complete\n");
    freeNetworkData(nD);
    free(nD);
    freeCfgData(&cfgData);
    return 0;

  netinfopubFail:
    freeNetworkData(nD);
    free(nD);
    freeCfgData(&cfgData);
    return 1;
  }

  else {
    printf("Unknown arguments\n");
    freeCfgData(&cfgData);
    return 1;
  }
}
