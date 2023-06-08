#ifndef __INCLUDE_GPIO__
#define __INCLUDE_GPIO__

int modemPowerToggle(char *pathToChip, int pinNum);

int checkPinVal(char *pathToChip, int pinNum);

#endif
