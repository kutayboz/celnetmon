#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static char configFilename[] = "celnetmon.cfg";

char *listStrDataNames[] = {
    "path_to_serial_port", "path_to_gpio_chip", "mqtt_url",
    "network_apn",         "mqtt_username",     "mqtt_password",
    "mqtt_root_topic",     "network_username",  "network_password",
    "network_auth"};
char *listIntDataNames[] = {"pin_pwr_key", "pin_status", "mqtt_port",
                            "network_operator"};

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

int readCfg(listCfgData *cfgData, const char *pathExe) {
  char lineBuffer[BUFSIZ];
  char *pathCfg, *pathDirEnd, *tmpPtr = NULL;
  int sizePathDir, idx, idx2;
  unsigned long numCharRead;
  FILE *fileCfg;

  /* Form config filepath string. */
  pathDirEnd = strrchr(pathExe, '/');
  sizePathDir = pathDirEnd - pathExe;
  if (pathDirEnd == NULL) {
    printf("Cannot obtain the executable directory.\n");
    return 1;
  }
  pathCfg = malloc(sizeof(char) * (sizePathDir + strlen(configFilename) + 2));
  strncpy(pathCfg, pathExe, sizePathDir + 1);
  strcat(pathCfg, configFilename);

  /* Open the config file. */
  fileCfg = fopen(pathCfg, "r");
  if (fileCfg == NULL) {
    printf("Cannot open config file \"%s\".\n", pathCfg);
    free(pathCfg);
    return 1;
  }

  /* Check that it is not empty. */
  if (fgets(lineBuffer, BUFSIZ, fileCfg) == NULL) {
    printf("Config file is empty.\n");
    fclose(fileCfg);
    free(pathCfg);
    return 1;
  }

  /* Allocate memory for the config data list. */
  cfgData->strDataList = malloc(STR_LIST_LENGTH * sizeof(strData));
  cfgData->intDataList = malloc(INT_LIST_LENGTH * sizeof(intData));

  /* Read the file and populate the config string data. */
  for (idx = 0; idx < STR_LIST_LENGTH; idx++) {
    cfgData->strDataList[idx].name =
        malloc(sizeof(char) * (strlen(listStrDataNames[idx]) + 1));
    strcpy(cfgData->strDataList[idx].name, listStrDataNames[idx]);
    cfgData->strDataList[idx].value = NULL;

    rewind(fileCfg);
    do {
      fgets(lineBuffer, BUFSIZ, fileCfg);
      tmpPtr = strtok(lineBuffer, "=");
      if (tmpPtr == NULL)
        continue;
      if (tmpPtr[0] == '/' && tmpPtr[1] == '/')
        continue;
      if (strstr(tmpPtr, cfgData->strDataList[idx].name) == NULL)
        continue;

      do {
        tmpPtr = strtok(NULL, "\"");
      } while (tmpPtr != NULL && *tmpPtr == ' ');
      if (tmpPtr == NULL)
        continue;

      cfgData->strDataList[idx].value =
          malloc(sizeof(char) * (strlen(tmpPtr) + 1));
      strcpy(cfgData->strDataList[idx].value, tmpPtr);
      break;
    } while (feof(fileCfg) == 0);
    if (cfgData->strDataList[idx].value == NULL) {
      if (idx < NETWORK_USERNAME_POS) {
        printf("Error parsing the config file.\nCould not find the \"%s\" "
               "variable.\n",
               cfgData->strDataList[idx].name);
        for (idx2 = 0; idx2 <= idx; idx2++) {
          free(cfgData->strDataList[idx2].name);
          free(cfgData->strDataList[idx2].value);
        }
        free(cfgData->strDataList);
        fclose(fileCfg);
        free(pathCfg);
        return 1;
      } else {
        cfgData->strDataList[idx].value =
            realloc(cfgData->strDataList[idx].value, sizeof(char));
        strcpy(cfgData->strDataList[idx].value, "");
      }
    }
  }

  /* Populate the config integer data names. */
  for (idx = 0; idx < INT_LIST_LENGTH; idx++) {
    cfgData->intDataList[idx].name =
        malloc(sizeof(char) * (strlen(listIntDataNames[idx]) + 1));
    strcpy(cfgData->intDataList[idx].name, listIntDataNames[idx]);
    cfgData->intDataList[idx].value = -1;

    rewind(fileCfg);
    do {
      fgets(lineBuffer, BUFSIZ, fileCfg);
      tmpPtr = strtok(lineBuffer, "=");
      if (tmpPtr == NULL)
        continue;
      if (tmpPtr[0] == '/' && tmpPtr[1] == '/')
        continue;
      if (strstr(tmpPtr, cfgData->intDataList[idx].name) == NULL)
        continue;

      do {
        tmpPtr = strtok(NULL, "\"");
      } while (tmpPtr != NULL && *tmpPtr == ' ');
      if (tmpPtr == NULL)
        continue;

      sscanf(tmpPtr, "%d%ln", &cfgData->intDataList[idx].value, &numCharRead);
      if (numCharRead != strlen(tmpPtr)) {
        printf("Format error for \"%s\".\n", cfgData->intDataList[idx].name);
        for (idx2 = 0; idx2 <= idx; idx2++) {
          free(cfgData->intDataList[idx2].name);
        }
        fclose(fileCfg);
        free(pathCfg);
        return 1;
      }
      break;
    } while (feof(fileCfg) == 0);
    if ((cfgData->intDataList[idx].value == -1) &&
        (idx < NETWORK_OPERATOR_POS)) {
      printf("Error parsing the config file.\nCould not find the \"%s\" "
             "variable.\n",
             cfgData->intDataList[idx].name);
      for (idx2 = 0; idx2 <= idx; idx2++) {
        free(cfgData->intDataList[idx2].name);
      }
      free(cfgData->strDataList);
      free(cfgData->intDataList);
      fclose(fileCfg);
      free(pathCfg);
      return 1;
    }
  }

  free(pathCfg);
  fclose(fileCfg);
  return 0;
}

int freeCfgData(listCfgData *cfgData) {
  int idx;
  for (idx = 0; idx < STR_LIST_LENGTH; idx++) {
    free(cfgData->strDataList[idx].name);
    free(cfgData->strDataList[idx].value);
  }
  free(cfgData->strDataList);

  for (idx = 0; idx < INT_LIST_LENGTH; idx++) {
    free(cfgData->intDataList[idx].name);
    cfgData->intDataList[idx].value = -1;
  }
  free(cfgData->intDataList);
  return 0;
}
