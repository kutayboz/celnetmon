#ifndef __INCLUDE_BG96__
#define __INCLUDE_BG96__

#include <time.h>

typedef struct networkDataTag {
  char *operatorName, *networkName, *networkRegStat, *trkAreaCode, *cellID,
      *tech, *band, *channel, *rssi, *rsrp, *sinr, *rsrq, *ber, *lat, *longt,
      *alt, *gpsTime;
  struct tm netTime;
} networkData;

int initNetworkData(networkData *nD);

int freeNetworkData(networkData *nD);

int powerOn(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int powerOff(char *pathToPort, char *pathToChip, int statusPin, int pwrKeyPin);

int cfgPublicNetwork(char *pathToPort);

int waitForNetwork(char *pathToPort, int waitTimeSeconds);

int mqttDisc(char *pathToPort);

int mqttConn(char *pathToPort, char *hostURL, int hostPort, char *username,
             char *password);

int mqttPubNetData(char *pathToPort, networkData nD);

int gatherData(networkData *output, char *pathToPort);

int initGNSS(char *pathToPort);

int stopGNSS(char *pathToPort);

#endif
