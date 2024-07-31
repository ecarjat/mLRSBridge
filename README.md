# mLRS Bridge #

This project based on https://github.com/olliw42/mLRS/tree/main/esp enables advanced communication between the mLRS tx and a computer. This communication enables the following functions

- Mavlink UDP bridging between tx transmitter and a GCS.
- access to the CLI when using a firmware that enables the CLI based on a PIN being driven low.
- flashing of the firmware on the mLRS TX

This currently only has been tested on the [easysolder board](https://github.com/olliw42/mLRS-docu/blob/master/docs/E77_EASYSOLDER.md) and requires a special firmware that can compiled by downloading [this fork](https://github.com/ecarjat/mLRS) of mLRS 

## Mavlink bridge ##

Connect to mLRS AP UDP wifi AP. The bridge broadcasts UDP messages from the tx and send the GCS messages back to the tx via UDP.

## Access to the CLI ##

Access to the CLI is provided using a REST API. Messages are sent to the cli using a post request to the ESP URL.
- API URL `http://[ESP_IP]/api/v1/cli`
- Verbs: `write`, `read`

### Example ####

Request version

```
curl --header "Content-Type: application/json" \
  --request POST \
  --data '{"write":"v;"}' \
  http://192.168.4.55:80/api/v1/cli
```

Read response

```
curl --header "Content-Type: application/json" \
  --request POST \
  --data '{"read":""}' \
  http://192.168.4.55:80/api/v1/cli
```

## Firmware flashing ##

Firmware flashing is done using a simple webserver located on the ESP32. The server can be accessed from the following URL http://[ESP_IP]

The firmware is first uploaded to the ESP32 and then flashed to the tx module. 

In order for the flashing to work a firmware that inverts the standard TX/RX and the JR pins first needs to be flashed. This is because the serial boot pins of the STM32 are located on PB6/PB7.


## Wiring ##

| ESP32 PIN | tx mLRS pin |
|-----------|-------------|
|5| E1 |
|16|S|
|17|E0|
|GND|G|

| Transmiter | tx mLRS pin|
|------------|------------|
| JR Pin| TX|




