#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

int checkPinVal(char *pathToChip, int pinNum) {
  int pinVal = -1;
  struct gpiod_chip *chip;
  struct gpiod_line *line;

  chip = gpiod_chip_open(pathToChip);
  if (!chip) {
    perror("Error opening GPIO chip");
    return -1;
  }

  line = gpiod_chip_get_line(chip, pinNum);
  if (!line) {
    perror("Error getting GPIO line");
    gpiod_chip_close(chip);
    return -1;
  }

  if (gpiod_line_request_input(line, "pin status") < 0) {
    perror("Error setting GPIO line as input");
    gpiod_chip_close(chip);
    return -1;
  }

  pinVal = gpiod_line_get_value(line);
  if (pinVal < 0) {
    perror("Error getting GPIO line value");
    gpiod_chip_close(chip);
    return -1;
  }

  gpiod_chip_close(chip);
  return pinVal;
}

int readCfg(struct structCfg *dataCfg, const char *pathExe) {
  char strBuffer[BUFSIZ];
  char *pathCfg, *pathDirEnd, *tmpPtr = NULL;
  const char *filenameCfg = "network_monitor.cfg";
  int sizePathDir;
  unsigned long numCharRead;
  FILE *fileCfg;

  /* Form config filepath string. */
  pathDirEnd = strrchr(pathExe, '/');
  sizePathDir = pathDirEnd - pathExe;
  if (pathDirEnd == NULL) {
    printf("Cannot obtain the executable directory.\n");
    return 1;
  }
  pathCfg = malloc(sizeof(char) * (sizePathDir + strlen(filenameCfg) + 2));
  strncpy(pathCfg, pathExe, sizePathDir + 1);
  strcat(pathCfg, filenameCfg);

  /* Open the config file. */
  fileCfg = fopen(pathCfg, "r");
  if (fileCfg == NULL) {
    printf("Cannot open config file \"%s\".\n", pathCfg);
    goto readCfgFail1;
  }

  /* Check that it is not empty. */
  if (fgets(strBuffer, BUFSIZ, fileCfg) == NULL) {
    printf("Config file is empty.\n");
    goto readCfgFail2;
  }

  /* Read the file and look for the "path_to_serial_port". */
  rewind(fileCfg);
  do {
    fgets(strBuffer, BUFSIZ, fileCfg);
    tmpPtr = strtok(strBuffer, " =\n\t\r");
    if (tmpPtr == NULL)
      continue;
    if (strcmp("path_to_serial_port", tmpPtr) != 0)
      continue;

    tmpPtr = strtok(NULL, " =\n\t\r\"");
    if (tmpPtr == NULL)
      continue;

    dataCfg->pathSerialPort =
        realloc(dataCfg->pathSerialPort, sizeof(char) * (strlen(tmpPtr) + 1));
    strcpy(dataCfg->pathSerialPort, tmpPtr);
    break;
  } while (feof(fileCfg) == 0);
  if (dataCfg->pathSerialPort == NULL) {
    printf("Could not find the \"path_to_serial_port\" variable.\n");
    goto readCfgFail2;
  }

  /* Read the file and look for the "path_to_gpio_chip". */
  rewind(fileCfg);
  do {
    fgets(strBuffer, BUFSIZ, fileCfg);
    tmpPtr = strtok(strBuffer, " =\n\t\r");
    if (tmpPtr == NULL)
      continue;
    if (strcmp("path_to_gpio_chip", tmpPtr) != 0)
      continue;

    tmpPtr = strtok(NULL, " =\n\t\r\"");
    if (tmpPtr == NULL)
      continue;

    dataCfg->pathGpioChip =
        realloc(dataCfg->pathGpioChip, sizeof(char) * (strlen(tmpPtr) + 1));
    strcpy(dataCfg->pathGpioChip, tmpPtr);
    break;
  } while (feof(fileCfg) == 0);
  if (dataCfg->pathGpioChip == NULL) {
    printf("Could not find the \"path_to_gpio_chip\" variable.\n");
    goto readCfgFail3;
  }

  /* Read the file and look for the "pin_pwr_key". */
  rewind(fileCfg);
  do {
    fgets(strBuffer, BUFSIZ, fileCfg);
    tmpPtr = strtok(strBuffer, " =\n\t\r");
    if (tmpPtr == NULL)
      continue;
    if (strcmp("pin_pwr_key", tmpPtr) != 0)
      continue;

    tmpPtr = strtok(NULL, " =\n\t\r\"");
    if (tmpPtr == NULL)
      continue;

    sscanf(tmpPtr, "%d%ln", &dataCfg->pinPwrKey, &numCharRead);
    if ((numCharRead != strlen(tmpPtr)) || (strtok(NULL, " =\n\t\r\"") != NULL)) {
      printf("Format error for \"pin_pwr_key\".\n");
      goto readCfgFail4;
    }
    break;
  } while (feof(fileCfg) == 0);
  if (dataCfg->pinPwrKey == -1) {
    printf("Error parsing the config file.\nCould not find the "
           "\"pin_pwr_key\" variable.\n");
    goto readCfgFail4;
  }

  /* Read the file and look for the "pin_status". */
  rewind(fileCfg);
  do {
    fgets(strBuffer, BUFSIZ, fileCfg);
    tmpPtr = strtok(strBuffer, " =\n\t\r");
    if (tmpPtr == NULL)
      continue;
    if (strcmp("pin_status", tmpPtr) != 0)
      continue;

    tmpPtr = strtok(NULL, " =\n\t\r\"");
    if (tmpPtr == NULL)
      continue;

    sscanf(tmpPtr, "%d%ln", &dataCfg->pinStatus, &numCharRead);
    if ((numCharRead != strlen(tmpPtr)) || (strtok(NULL, " =\n\t\r\"") != NULL)) {
      printf("Format error for \"pin_status\".\n");
      goto readCfgFail4;
    }
    break;
  } while (feof(fileCfg) == 0);
  if (dataCfg->pinStatus == -1) {
    printf("Error parsing the config file.\nCould not find the "
           "\"pin_status\" variable.\n");
    goto readCfgFail4;
  }

  free(pathCfg);
  fclose(fileCfg);
  return 0;

readCfgFail4:
  free(dataCfg->pathGpioChip);
readCfgFail3:
  free(dataCfg->pathSerialPort);
readCfgFail2:
  fclose(fileCfg);
readCfgFail1:
  free(pathCfg);
  return 1;
}

/* Do not call this before initializing dataCfg with readCfg(). */
int freeCfgData(struct structCfg *dataCfg) {
  dataCfg->pinPwrKey = -1;
  dataCfg->pinStatus = -1;
  free(dataCfg->pathGpioChip);
  free(dataCfg->pathSerialPort);
  return 0;
}
