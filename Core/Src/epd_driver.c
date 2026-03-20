#include "epd_driver.h"
#include <stdint.h>

static void EPD_SPI_WriteByte(uint8_t data) {
    HAL_SPI_Transmit(&hspi4, &data, 1,1);
}

static void EPD_WriteReg(uint8_t cmd){ 
    EPD_DC_0; 
    EPD_CS_0;
    EPD_SPI_WriteByte(cmd);
    EPD_CS_1;
}

static void EPD_WriteData(uint8_t data){
    EPD_DC_1;
    EPD_CS_0;
    EPD_SPI_WriteByte(data);
    EPD_CS_1;
}

static void EPD_WaitUntilIdle(void) {
    while(EPD_BUSY_READ == 1 ) {
        HAL_Delay(10); 
    }
}

static void EPD_Reset(void){
    EPD_RST_1;
    HAL_Delay(20);
    EPD_RST_0;
    HAL_Delay(2);
    EPD_RST_1;
    HAL_Delay(20);
}

void EPD_Init(void) {
    EPD_Reset();
    EPD_WaitUntilIdle();

    EPD_WriteReg(0x12); 
    EPD_WaitUntilIdle();


    EPD_WriteReg(0x01); 
    EPD_WriteData(0x27);
    EPD_WriteData(0x01);
    EPD_WriteData(0x00);


    EPD_WriteReg(0x11); 
    EPD_WriteData(0x03); 


    EPD_WriteReg(0x44); 
    EPD_WriteData(0x00);
    EPD_WriteData(EPD_WIDTH/8 - 1); 


    EPD_WriteReg(0x45); 
    EPD_WriteData(0x00);
    EPD_WriteData(0x00);
    EPD_WriteData((EPD_HEIGHT-1) & 0xFF); 
    EPD_WriteData((EPD_HEIGHT-1) >> 8);   


    EPD_WriteReg(0x3C); 
    EPD_WriteData(0x05);

   
    EPD_WriteReg(0x21); 
    EPD_WriteData(0x00); 
    EPD_WriteData(0x80);
}

void EPD_Display(const uint8_t *Image){
    uint16_t i;

    EPD_WriteReg(0x24); 
    for (i = 0; i < ((EPD_WIDTH / 8) * EPD_HEIGHT); i++) {
        EPD_WriteData(Image[i]);
    }

    EPD_WriteReg(0x22); 
    EPD_WriteData(0xF7); 
    EPD_WriteReg(0x20); 
    EPD_WaitUntilIdle();
}

void EPD_Clear(void) {
    uint16_t i;
    
    EPD_WriteReg(0x24);
    for (i = 0; i < ((EPD_WIDTH / 8) * EPD_HEIGHT); i++) {
        EPD_WriteData(0xFF); 
    }
    
    EPD_WriteReg(0x22); 
    EPD_WriteData(0xF7); 
    EPD_WriteReg(0x20); 
    EPD_WaitUntilIdle();
}

void EPD_Sleep(void) {
    EPD_WriteReg(0x10); 
    EPD_WriteData(0x01); 
}




