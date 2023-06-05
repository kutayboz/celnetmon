#include <gpiod.h>
#include <stdio.h>

int modemPowerToggle(int pinNum) {
  struct gpiod_chip *chip;
  struct gpiod_line *line;

  chip = gpiod_chip_open("/dev/gpiochip0");
  if (!chip) {
    perror("Error opening GPIO chip");
    return 1;
  }

  line = gpiod_chip_get_line(
      chip, pinNum); // Replace with the desired GPIO pin number
  if (!line) {
    perror("Error getting GPIO line");
    gpiod_chip_close(chip);
    return 1;
  }

  if (gpiod_line_request_output(line, "cellular modem power toggle", 0) < 0) {
    perror("Error setting GPIO line as output");
    gpiod_chip_close(chip);
    return 1;
  }

  if (gpiod_line_set_value(line, 1) < 0) {
    perror("Error setting GPIO line high");
    gpiod_chip_close(chip);
    return 1;
  }

  sleep(1);

  if (gpiod_line_set_value(line, 0) < 0) {
    perror("Error setting GPIO line low");
    gpiod_chip_close(chip);
    return 1;
  }

  gpiod_chip_close(chip);
  return 0;
}

int checkPinVal(int pinNum) {
  int pinVal = -1;
  struct gpiod_chip *chip;
  struct gpiod_line *line;

  chip = gpiod_chip_open("/dev/gpiochip0");
  if (!chip) {
    perror("Error opening GPIO chip");
    return -1;
  }

  line = gpiod_chip_get_line(
      chip, pinNum); // Replace with the desired GPIO pin number
  if (!line) {
    perror("Error getting GPIO line");
    gpiod_chip_close(chip);
    return -1;
  }

  if (gpiod_line_request_input(line, "cellular modem power status") < 0) {
    perror("Error setting GPIO line as input");
    gpiod_chip_close(chip);
    return -1;
  }

  pinVal = gpiod_line_get_value(line);
  if (pinVal < 0) {
    perror("Error getting GPIO line value");
    gpiod_chip_close(chip);
    return -1;
  }

  gpiod_chip_close(chip);
  return pinVal;
}
