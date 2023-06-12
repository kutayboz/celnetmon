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

  if (argc == 1) {
    printf("No arguments provided\n");
    return 1;
  }

  else if (0 == strcmp(argv[1], "--initpublic")) {

    printf("Powering on\n");
    if (0 != powerOn(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                     PIN_NUM_PWR_KEY)) {
      printf("Error powering the module on\n");
      return 1;
    }

    printf("Initiating GNSS\n");
    if (0 != initGNSS(PATH_TO_SERIAL_PORT)) {
      printf("Error initiating GNSS\n");
      return 1;
    }

    printf("Configuring the network settings for DNA public network\n");
    if (0 != cfgPublicNetwork(PATH_TO_SERIAL_PORT)) {
      printf("Error setting public network configuration\n");
      return 1;
    }

    printf("Waiting for establishing network connection\n");
    if (0 != waitForNetwork(PATH_TO_SERIAL_PORT, 120)) {
      printf("Error while waiting for network\n");
      return 1;
    }

    printf("Initialization complete\n");
    return 0;
  }

  else if (0 == strcmp(argv[1], "--init6GTN")) {

    printf("Powering on\n");
    if (0 != powerOn(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                     PIN_NUM_PWR_KEY)) {
      printf("Error powering the module on\n");
      return 1;
    }

    printf("Initiating GNSS\n");
    if (0 != initGNSS(PATH_TO_SERIAL_PORT)) {
      printf("Error initiating GNSS\n");
      return 1;
    }

    printf("Configuring the network settings for 6GTN\n");
    if (0 != cfg6GTN(PATH_TO_SERIAL_PORT)) {
      printf("Error setting 6GTN network configuration\n");
      return 1;
    }

    printf("Waiting for establishing network connection\n");
    if (0 != connect6GTN(PATH_TO_SERIAL_PORT, 120)) {
      printf("Error while connecting to 6GTN\n");
      return 1;
    }

    printf("Initialization complete\n");
    return 0;
  }

  else if (0 == strcmp(argv[1], "--stop")) {

    printf("Stopping GNSS\n");
    if (0 != stopGNSS(PATH_TO_SERIAL_PORT)) {
      printf("Error stopping GNSS\n");
    }

    printf("Disconnecting MQTT\n");
    if (0 != mqttDisc(PATH_TO_SERIAL_PORT)) {
      printf("Could not disconnect MQTT\n");
    }

    printf("Powering off\n");
    if (0 != powerOff(PATH_TO_SERIAL_PORT, PATH_TO_GPIO_CHIP, PIN_NUM_STATUS,
                      PIN_NUM_PWR_KEY)) {
      printf("Error powering the module off\n");
      return 1;
    }
    printf("Power off complete\n");
    return 0;
  }

  else if (0 == strcmp(argv[1], "--netinfo-pub")) {
    networkData *nD = malloc(sizeof(networkData));

    printf("Obtaining network info\n");
    initNetworkData(nD);
    if (gatherData(nD, PATH_TO_SERIAL_PORT) != 0) {
      printf("Failed to gather network info\n");
      goto netinfopubFail;
    }

    printf("Connecting to the MQTT broker\n");
    if (0 != mqttConn(PATH_TO_SERIAL_PORT, "io.adafruit.com", 8883, "kboz",
                      "aio_lvhz87FJ0rIlidH8g30V5mQ7FK7s")) {
      printf("Error connecting MQTT\n");
      goto netinfopubFail;
    }

    printf("Publishing network information\n");
    if (0 != mqttPubNetData(PATH_TO_SERIAL_PORT, *nD)) {
      printf("Error publishing to MQTT server\n");
      goto netinfopubFail;
    }

    printf("Disconnecting from the MQTT broker\n");
    if (0 != mqttDisc(PATH_TO_SERIAL_PORT)) {
      printf("Could not disconnect MQTT\n");
      goto netinfopubFail;
    }

    printf("Network info publish complete\n");
    freeNetworkData(nD);
    free(nD);
    return 0;

  netinfopubFail:
    freeNetworkData(nD);
    free(nD);
    return 1;
  }

  else {
    printf("Unknown arguments\n");
    return 1;
  }
}
