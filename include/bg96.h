#ifndef __INCLUDE_BG96__
#define __INCLUDE_BG96__

int powerOn(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int powerOff(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int setupPublicConnection(char *pathToPort);

int waitForConnection(char *pathToPort, int waitTimeSeconds);

int tcpipInit(char *pathToPort);

int tcpipClose(char *pathToPort);

#endif
