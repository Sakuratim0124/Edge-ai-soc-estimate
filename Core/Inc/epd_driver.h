#ifndef __EPD_DRIVER_H
#define __EPD_DRIVER_H

#include "main.h" 
#include "spi.h"  


#define EPD_CS_0    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define EPD_CS_1    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define EPD_DC_0    HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET) // 命令
#define EPD_DC_1    HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)   // 数据

#define EPD_RST_0   HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET)
#define EPD_RST_1   HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET)

#define EPD_BUSY_READ  HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin)


#define EPD_WIDTH   128
#define EPD_HEIGHT  296

void EPD_Init(void);
void EPD_Clear(void);
void EPD_Display(const uint8_t *Image);
void EPD_Sleep(void);


#endif
