#ifndef __INCLUDE_UTILS__
#define __INCLUDE_UTILS__

enum {
  /* Mandatory */
  PATH_TO_SERIAL_PORT_POS,
  PATH_TO_GPIO_CHIP_POS,
  MQTT_URL_POS,
  NETWORK_APN_POS,
  /* Optional */
  MQTT_USERNAME_POS,
  MQTT_PASSWORD_POS,
  MQTT_ROOT_TOPIC_POS,
  NETWORK_USERNAME_POS,
  NETWORK_PASSWORD_POS,
  NETWORK_AUTH_POS,
  STR_LIST_LENGTH
};

enum { /*Mandatory */
       PIN_PWR_KEY_POS,
       PIN_STATUS_POS,
       MQTT_PORT_POS,
       /* Optional */
       NETWORK_OPERATOR_POS,
       INT_LIST_LENGTH
};

typedef struct {
  char *name;
  char *value;
} strData;

typedef struct {
  char *name;
  int value;
} intData;

/* Initialize this with NULL and 0 values. */
typedef struct {
  strData *strDataList;
  intData *intDataList;
} listCfgData;

int checkPinVal(char *pathToChip, int pinNum);

/* Example config file:
path_to_serial_port =   "/dev/serial0"
path_to_gpio_chip   =   "/dev/gpiochip0"
pin_pwr_key =   "17"
pin_status  =   "27"
Below are optional:
network_username  = "something something"
network_password  = "something something"
network_auth  = "something something"
*/
int readCfg(listCfgData *cfgData, const char *pathExe);

int freeCfgData(listCfgData *cfgData);

#endif
