#ifndef __INCLUDE_BG96__
#define __INCLUDE_BG96__

struct networkData {
  char *operatorName;
  char *currentTech;
  char *networkRegStat;
  char *trkAreaCode;
  char *cellID;
  char *rssi;
  char *ber;
};

int initNetworkData(struct networkData *nD);

int freeNetworkData(struct networkData *nD);

int powerOn(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int powerOff(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int cfgPublicNetwork(char *pathToPort);

int waitForNetwork(char *pathToPort, int waitTimeSeconds);

int mqttDisc(char *pathToPort);

int mqttConn(char *pathToPort, char *hostURL, int hostPort, char *username,
             char *password);

int mqttPub(char *pathToPort, char *topic, char *msg);

int gatherData(struct networkData *output, char *pathToPort);

#endif
