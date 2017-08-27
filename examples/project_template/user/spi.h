#ifndef __SPI_H__
#define __SPI_H__

#include "esp_common.h"
#include "user_config.h"
#include "uart.h"
#include "spi_interface.h"

void spi_init();
void spi_send(unsigned char addr,unsigned char data);

#endif


