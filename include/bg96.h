#ifndef __INCLUDE_BG96__
#define __INCLUDE_BG96__

int powerOn(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int powerOff(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int cfgPublicNetwork(char *pathToPort);

int waitForNetwork(char *pathToPort, int waitTimeSeconds);

int mqttDisc(char *pathToPort);

int mqttConn(char *pathToPort, char *hostURL, int hostPort, char *username,
             char *password);

#endif
