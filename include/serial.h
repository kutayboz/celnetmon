#ifndef __INCLUDE_SERIAL__
#define __INCLUDE_SERIAL__

typedef enum { SUCCESS, ERROR_READ, ERROR_TIMEOUT } SERIAL_RETURN_CODE;

int openSerialPort(char pathToPort[]);

SERIAL_RETURN_CODE querySerialPort(char **output, int serialPort, char *input,
                                   int timeoutSeconds, char *lastLineSubStr);

#endif
