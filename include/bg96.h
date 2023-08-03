#ifndef __INCLUDE_BG96__
#define __INCLUDE_BG96__

#include "utils.h"
#include <time.h>

typedef struct networkDataTag {
  char *operatorName, *networkName, *networkRegStat, *trkAreaCode, *cellID,
      *tech, *band, *channel, *rssi, *rsrp, *sinr, *rsrq, *ber, *lat, *longt,
      *alt, *gpsTime;
  struct tm netTime;
} networkData;

int modemPowerToggle(listCfgData cfgData, int pinNum);

int initNetworkData(networkData *nD);

int freeNetworkData(networkData *nD);

int powerOn(listCfgData cfgData);

int powerOff(listCfgData cfgData);

int configureNetwork(listCfgData cfgData);

int cfg6GTN(listCfgData cfgData);

int defineNetworkDetails(listCfgData cfgData);

int waitForNetwork(listCfgData cfgData, int waitTimeSeconds);

int mqttDisc(listCfgData cfgData);

int mqttConn(listCfgData cfgData);

int mqttPubNetData(listCfgData cfgData, networkData nD);

int gatherData(networkData *output, listCfgData cfgData);

int initGNSS(listCfgData cfgData);

int stopGNSS(listCfgData cfgData);

#endif
