/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "user_config.h"
#include "uart.h"
#include "pwm.h"
#include "gpio.h"
#include "espconn.h"

#define SERVER_IP		"192.168.1.125"
#define SERVER_PORT		5000

unsigned char IsGotIP = 0;
unsigned char timeout = 0;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt)
{
	printf("event %x:", evt->event_id);
	switch (evt->event_id) {
		case EVENT_STAMODE_CONNECTED:
			printf("connect to ssid %s, channel %d\n",
			evt->event_info.connected.ssid,
			evt->event_info.connected.channel);
			break;
		case EVENT_STAMODE_DISCONNECTED:
			printf("disconnect from ssid %s, reason %d\n",
			evt->event_info.disconnected.ssid,
			evt->event_info.disconnected.reason);
			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			printf("mode: %d -> %d\n",
			evt->event_info.auth_change.old_mode,
			evt->event_info.auth_change.new_mode);
			break;
		case EVENT_STAMODE_GOT_IP:
			printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
			IP2STR(&evt->event_info.got_ip.ip),
			IP2STR(&evt->event_info.got_ip.mask),
			IP2STR(&evt->event_info.got_ip.gw));
			printf("\n");
			IsGotIP = 1;
			break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			printf("station: " MACSTR "join, AID = %d\n",
			MAC2STR(evt->event_info.sta_connected.mac),
			evt->event_info.sta_connected.aid);
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			printf("station: " MACSTR "leave, AID = %d\n",
			MAC2STR(evt->event_info.sta_disconnected.mac),
			evt->event_info.sta_disconnected.aid);
			break;
		default:
			break;
	}
}

void scan_done(void *arg, STATUS status)
{
	uint8 ssid[33];
	char temp[128];
	if (status == OK) {
		struct bss_info *bss_link = (struct bss_info *)arg;
		while (bss_link != NULL) {
			memset(ssid, 0, 33);
			if (strlen(bss_link->ssid) <= 32)
				memcpy(ssid, bss_link->ssid, strlen(bss_link->ssid));
			else
				memcpy(ssid, bss_link->ssid, 32);
			printf("(%d,\"%s\",%d,\""MACSTR"\",%d)\r\n",
					bss_link->authmode, ssid, bss_link->rssi,
					MAC2STR(bss_link->bssid),bss_link->channel);
			bss_link = bss_link->next.stqe_next;
		}
	} else {
		printf("scan fail !!!\r\n");
	}
}

void conn_recv_callback(void *arg, char *pdata, unsigned short len);
void conn_sent_callback(void *arg);

esp_tcp tcp_con = 
{
	.remote_port = 5000,
	.local_port = 5000,
	.local_ip = {0,0,0,0},	
	.remote_ip = {192,168,1,125},	
};

struct espconn esp_connection = 
{
	.type = ESPCONN_TCP,
	.proto.tcp = &tcp_con,
	.recv_callback = conn_recv_callback,	
	.sent_callback = conn_sent_callback,
};

void conn_recv_callback(void *arg, char *pdata, unsigned short len)
{
	int i;
	printf("conn_recv_callback\r\n");
	for(i=0;i<len;i++)
	{
		printf("%d ",pdata[i]);
	}
	printf("\r\n");

}

void conn_sent_callback(void *arg)
{
	printf("conn_sent_callback\r\n");
	
}

void task1(void *pvParameters)
{
	int i,a = 0,sta_socket;
	STATION_STATUS station_status;
	esp_tcp tcp_con;
	struct station_config * config = (struct station_config *)zalloc(sizeof(struct station_config));
	sprintf(config->ssid, "FAST_A024D0");
	sprintf(config->password, "tangquan");
	wifi_set_opmode(STATION_MODE);
	wifi_station_set_config_current(config);
	wifi_station_connect();
	free(config);	
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	wifi_station_scan(NULL,scan_done);
	
	while(1)
	{
		station_status = wifi_station_get_connect_status();
		switch (station_status)
		{
			case STATION_IDLE:
				printf("STATION_IDLE\r\n");
				break;
			case STATION_CONNECTING:
				printf("STATION_CONNECTING\r\n");
				break;
			case STATION_WRONG_PASSWORD:
				printf("STATION_WRONG_PASSWORD\r\n");
				break;
			case STATION_NO_AP_FOUND:
				printf("STATION_NO_AP_FOUND\r\n");
				break;
			case STATION_CONNECT_FAIL:
				printf("STATION_CONNECT_FAIL\r\n");
				break;
			case STATION_GOT_IP:
				printf("STATION_GOT_IP\r\n");
				goto STATION_GOT_IP;
				break;
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
	
STATION_GOT_IP:
	espconn_init();
	if(espconn_connect(&esp_connection) == 0)
	{
		printf("espconn_connect success\r\n");
 		while(1)
		{
			if(++timeout == 10)
				break;
			if(esp_connection.state == ESPCONN_CONNECT)
			{
				printf("connected to server\r\n");
				break;
			}
			vTaskDelay(1000/portTICK_RATE_MS);
		}

		if(timeout == 10)
		{
			printf("connection timeout\r\n");
			espconn_disconnect(&esp_connection);
		}
		else
		{
			espconn_send(&esp_connection,"tangquan",8);
		}
		
	}
	else
	{
		printf("espconn_connect failed\r\n");
	}

	
	while (1)
	{
		// printf("tangquan\r\n");
		
		a = ~a;
		if(a == 0)
			gpio_output_set(0, BIT2, BIT2, 0);
		else
			gpio_output_set(BIT2, 0, BIT2, 0);
	

		for(i=0;i<1000;i++)
			os_delay_us(1000);
	}
	vTaskDelete(NULL);
}

void UART_intr_handler()
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
    * uart1 and uart0 respectively
    */
    uint8 RcvChar;
    uint8 uart_no = UART0;//UartDev.buff_uart_no;
    uint8 fifo_len = 0;
    uint8 buf_idx = 0;
    uint8 fifo_tmp[128] = {0};

    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;

    while (uart_intr_status != 0x0)
    {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST))
        {
            //printf("FRM_ERR\r\n");
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
        }
        else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) 
        {
            //printf("full\r\n");
            fifo_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
            buf_idx = 0;

            while (buf_idx < fifo_len) 
            {
                uart_tx_one_char(UART0, READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
                buf_idx++;
            }

            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
        } 
        else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST))
        {
            //printf("tout\r\n");
            fifo_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
            buf_idx = 0;

            while (buf_idx < fifo_len)
            {
                uart_tx_one_char(UART0, READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
                buf_idx++;
            }

            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
        }
        else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST))
        {
            printf("empty\n\r");
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
        }
        else 
        {
            //skip
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{	
	UART_SetBaudrate(UART0,BIT_RATE_115200);
	UART_SetPrintPort(UART0);
//	UART_SetIntrEna(UART0,UART_INTR_MASK);
	ETS_UART_INTR_ENABLE();
	UART_intr_handler_register(UART_intr_handler,NULL);
	
	os_delay_us(10000);
	
	printf("\r\n\r\n------ESP8266 SDK development test------\r\n\r\n");
	printf("SDK version:%s\r\n", system_get_sdk_version());
	printf("ESP8266 chip ID:0x%x\n", system_get_chip_id());
	system_print_meminfo();
	
	
	
	xTaskCreate(task1, "task1", 256, NULL, 2, NULL);
	
}
















