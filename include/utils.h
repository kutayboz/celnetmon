#ifndef __INCLUDE_UTILS__
#define __INCLUDE_UTILS__

struct structCfg {
  char *pathSerialPort;
  char *pathGpioChip;
  int pinPwrKey, pinStatus;
};

int checkPinVal(char *pathToChip, int pinNum);

int readCfg(struct structCfg *dataCfg, const char *pathExe);

int freeCfgData(struct structCfg *dataCfg);

#endif
