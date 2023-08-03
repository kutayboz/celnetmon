# CelNetMon - Cellular Network Monitor
A cellular network signal quality monitoring utility. Written to be used during my thesis work.

Uses the BG96 chip with AT commands, connected through GPIO and serial pins of a Linux machine. Should be easy to adapt the code to other devices using AT commands.

Requires `libgpiod`. Compiled and tested with Clang 11.0.1 aarch64.

## Compiling
CMake is used in development. For development, `celnetmon` target can be used to work in the build folder. For release, `install` target creates and puts the executable in `./bin`.

## Usage
Make sure to create a `celnetmon.cfg` config file in the same folder with the executable. An example `celnetmon.cfg.example` is included in the repo.

- `celnetmon --init`
Powers the BG96 chip on, initiates the GNSS system, configures and connects to the network.

- `celnetmon --publish`
Collects network telemetry and GNSS location, publishes the data to the specified MQTT server in the config file.

- `celnetmon --stop`
Powers the chip down.

> [!NOTE]
> A-GPS technologies are not implemented. For that reason, GNSS takes some time to establish.

Telemetry data that is collected are:
- Network operator name
- Roaming status
- Tracking area code
- Cell ID
- Technology
- Band
- Channel
- Network Time
- GPS location
- Signal quality indicators (BER, RSRP, RSRQ, RSSI, SINR)