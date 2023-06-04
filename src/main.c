#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gpio.h>
#include <serial.h>

int main() {

  char *commandList[] = {"AT", "ATI", "AT+COPS?"/* ,
                         "AT+COPS=?" */},
       serialFilePath[] = "/dev/serial0", *output = malloc(1);

  int serialPort;
  unsigned long idx;

  serialPort = openSerialPort(serialFilePath);
  if (serialPort < 0) {
    printf("Error openSerialPort()\n");
    free(output);
    return 1;
  }

  /* if (0 != querySerialPort(&output, serialPort, "AT")) {
    printf("Error querySerialPort()\n");
    close(serialPort);
    free(output);
    return 1;
  }
  if (NULL == strstr(output, "OK\r\n")) {
    if (0 != modemPowerToggle(17)) {
      printf("Error toggling modem power\n");
      free(output);
      return 1;
    }
  } */

  for (idx = 0; idx < sizeof(commandList) / sizeof(*commandList); idx++) {
    if (0 != querySerialPort(&output, serialPort, commandList[idx])) {
      printf("Error querySerialPort()\n");
      close(serialPort);
      free(output);
      return 1;
    }
    printf("%s\n", output);
  }

  close(serialPort);

  /* if (0 != modemPowerToggle(17)) {
    printf("Error toggling modem power\n");
    free(output);
    return 1;
  } */

  free(output);
  return 0;
}
