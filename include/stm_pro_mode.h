// Imported from 
// https://github.com/ESP32-Musings/OTA_update_STM32_using_ESP32
/*
MIT License

Copyright (c) 2020 Laukik Hase

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _STM_PRO_MODE_H
#define _STM_PRO_MODE_H

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_system.h"
#include "esp_spiffs.h"

#include "esp_event.h"
#include "esp_wifi.h"


#include "nvs_flash.h"
#include "logger.h"

//Macro for error checking
#define IS_ESP_OK(x) if ((x) != ESP_OK) break;

#define UART_BUF_SIZE 1024

#define ACK 0x79
#define SERIAL_TIMEOUT 5000


//Increment the memory address for the next write operation
void incrementLoadAddress(char *loadAddr);

//End the connection with STM32Fxx
void endConn(void);

//Get in sync with STM32
int cmdSync(uint8_t nb_uart);

//Get the version and the allowed commands supported by the current version of the bootloader
int cmdGet(uint8_t nb_uart);

//Get the bootloader version and the Read Protection status of the Flash memory
int cmdVersion(uint8_t nb_uart);

//Get the chip ID
int cmdId(uint8_t nb_uart);

//Erase from one to all the Flash memory pages
int cmdErase(uint8_t nb_uart);

//Erases from one to all the Flash memory pages using 2-byte addressing mode
int cmdExtErase(uint8_t nb_uart);

//Setup STM32Fxx for the 'flashing' process
void setupSTM(uint8_t nb_uart);

//Write data to flash memory address
int cmdWrite(uint8_t nb_uart);

//Read data from flash memory address
int cmdRead(uint8_t nb_uart);

//UART send data to STM32Fxx & wait for response
int sendBytes(uint8_t nb_uart, const char *bytes, int count, int resp);

//UART send data byte-by-byte to STM32Fxx
int sendData(uint8_t nb_uart, const char *logName, const char *data, const int count);

//Wait for response from STM32Fxx
int waitForSerialData(uint8_t nb_uart, int dataCount, int timeout);

//Send the STM32Fxx the memory address, to be written
int loadAddress(uint8_t nb_uart, const char adrMS, const char adrMI, const char adrLI, const char adrLS);

//UART write the flash memory address of the STM32Fxx with blocks of data 
esp_err_t flashPage(uint8_t nb_uart, const char *address, const char *data);

//UART read the flash memory address of the STM32Fxx and verify with the given block of data 
esp_err_t readPage(uint8_t nb_uart, const char *address, const char *data);

#endif
