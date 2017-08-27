#include "spi.h"

SpiAttr spi_attr;

void spi_isrFunc(void* data)
{

}

void spi_init()
{
	printf("%s,%s,%d:spi_init\r\n",__FILE__,__FUNCTION__,__LINE__);
	
	spi_attr.mode		= SpiMode_Master;
	spi_attr.subMode	= SpiSubMode_0;
	spi_attr.speed		= SpiSpeed_20MHz;
	spi_attr.bitOrder	= SpiBitOrder_MSBFirst;

    // Init HSPI GPIO
    WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode

    printf("\r\n ============= spi init slave =============\r\n");
	SPIInit(SpiNum_HSPI,&spi_attr);
	
//	printf("%s,%s,%d:spi_init\r\n",__FILE__,__FUNCTION__,__LINE__);
//	SPICsPinSelect(SpiNum_HSPI,SpiPinCS_0);	
//	printf("%s,%s,%d:spi_init\r\n",__FILE__,__FUNCTION__,__LINE__);
//
//    SPISlaveRecvData(SpiNum_HSPI,spi_isrFunc);
//	printf("%s,%s,%d:spi_init\r\n",__FILE__,__FUNCTION__,__LINE__);
//    uint32_t sndData[8] = { 0 };
//    sndData[0] = 0x35343332;
//    sndData[1] = 0x39383736;
//    sndData[2] = 0x3d3c3b3a;
//    sndData[3] = 0x11103f3e;
//    sndData[4] = 0x15141312;
//    sndData[5] = 0x19181716;
//    sndData[6] = 0x1d1c1b1a;
//    sndData[7] = 0x21201f1e;
//
//    SPISlaveSendData(SpiNum_HSPI, sndData, 8);
//	printf("%s,%s,%d:spi_init\r\n",__FILE__,__FUNCTION__,__LINE__);
//    WRITE_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI), 0x8A);
//    WRITE_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI), 0x83);
//	printf("%s,%s,%d:spi_init\r\n",__FILE__,__FUNCTION__,__LINE__);
}

void spi_send(unsigned char addr,unsigned char data)
{
	printf("spi_send\r\n");

	SPIMasterCfgAddr(SpiNum_HSPI,addr);
	SPIMasterCfgCmd(SpiNum_HSPI,data);
	// SPIMasterSendData(SpiNum_HSPI,data);
}








