#ifndef __INCLUDE_SERIAL__
#define __INCLUDE_SERIAL__

int openSerialPort(char pathToPort[]);

int querySerialPort(char **output, int serialPort, char *input,
                    int timeoutSeconds);

#endif
