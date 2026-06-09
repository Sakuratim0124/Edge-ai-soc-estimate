
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

 /*
  * Description
  *   v1.0 - Minimum template to show how to use the Embedded Client API
  *          model. Only one input and one output is supported. All
  *          memory resources are allocated statically (AI_NETWORK_XX, defines
  *          are used).
  *          Re-target of the printf function is out-of-scope.
  *   v2.0 - add multiple IO and/or multiple heap support
  *
  *   For more information, see the embeded documentation:
  *
  *       [1] %X_CUBE_AI_DIR%/Documentation/index.html
  *
  *   X_CUBE_AI_DIR indicates the location where the X-CUBE-AI pack is installed
  *   typical : C:\Users\[user_name]\STM32Cube\Repository\STMicroelectronics\X-CUBE-AI\7.1.0
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#endif

/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "network.h"
#include "network_data.h"

/* USER CODE BEGIN includes */
/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/

#if !defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_in_1[AI_NETWORK_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_NETWORK_IN_NUM] = {
data_in_1
};
#else
ai_i8* data_ins[AI_NETWORK_IN_NUM] = {
NULL
};
#endif

#if !defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_out_1[AI_NETWORK_OUT_1_SIZE_BYTES];
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = {
data_out_1
};
#else
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = {
NULL
};
#endif

/* Activations buffers -------------------------------------------------------*/

AI_ALIGNED(32)
static uint8_t pool0[AI_NETWORK_DATA_ACTIVATION_1_SIZE];

ai_handle data_activations0[] = {pool0};

/* AI objects ----------------------------------------------------------------*/

static ai_handle network = AI_HANDLE_NULL;

static ai_buffer* ai_input;
static ai_buffer* ai_output;

static void ai_log_err(const ai_error err, const char *fct)
{
  /* USER CODE BEGIN log */
  if (fct)
    printf("TEMPLATE - Error (%s) - type=0x%02x code=0x%02x\r\n", fct,
        err.type, err.code);
  else
    printf("TEMPLATE - Error - type=0x%02x code=0x%02x\r\n", err.type, err.code);

  do {} while (1);
  /* USER CODE END log */
}

static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err;

  /* Create and initialize an instance of the model */
  err = ai_network_create_and_init(&network, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_network_create_and_init");
    return -1;
  }

  ai_input = ai_network_inputs_get(network, NULL);
  ai_output = ai_network_outputs_get(network, NULL);

#if defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-inputs" option is used, memory buffer can be
   *  used from the activations buffer. This is not mandatory.
   */
  for (int idx=0; idx < AI_NETWORK_IN_NUM; idx++) {
	data_ins[idx] = ai_input[idx].data;
  }
#else
  for (int idx=0; idx < AI_NETWORK_IN_NUM; idx++) {
	  ai_input[idx].data = data_ins[idx];
  }
#endif

#if defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-outputs" option is used, memory buffer can be
   *  used from the activations buffer. This is no mandatory.
   */
  for (int idx=0; idx < AI_NETWORK_OUT_NUM; idx++) {
	data_outs[idx] = ai_output[idx].data;
  }
#else
  for (int idx=0; idx < AI_NETWORK_OUT_NUM; idx++) {
	ai_output[idx].data = data_outs[idx];
  }
#endif

  return 0;
}

static int ai_run(void)
{
  ai_i32 batch;

  batch = ai_network_run(network, ai_input, ai_output);
  if (batch != 1) {
    ai_log_err(ai_network_get_error(network),
        "ai_network_run");
    return -1;
  }

  return 0;
}

/* USER CODE BEGIN 2 */

// 標準化參數（從 JSON 取得）

static const float x_mean[] = {3.597097635269165f, -1.4133143424987793f};
static const float x_std[] = {0.2873367667198181f, 0.0047073024325072765f};
static const float y_mean = 50.30403518676758f;
static const float y_std = 28.806041717529297f;

// 外部變數（在 main.c 定義）
extern float voltage, Curr[20];
extern int soc, soh;
extern uint8_t epd_update_flag;
extern char meassage[256];
extern UART_HandleTypeDef huart1;

// 資料緩衝（50 個時間步 × 2 通道）
static float data_buf[50][2];
static uint16_t data_count = 0;
static uint16_t inference_counter = 0;

int acquire_and_process_data(ai_i8* data[])
{
  /* 擷取 V 與 I，填滿緩衝區，當收集到 20 個樣本時進行標準化並填入 AI 輸入 */
  
  // 每次呼叫時從 main.c 的全域變數讀取 V 與 I
  // (注意：此處假設 voltage 與 Curr[0] 已由 RTC 中斷或主迴圈更新)
  
  // 填充緩衝區
  if (data_count < 50) {
    data_buf[data_count][0] = voltage;    // 通道0: 電壓
    data_buf[data_count][1] = Curr[0];    // 通道1: 電流
    
    // 分離整數和小數部分 (避免浮點格式問題)
    int v_int = (int)voltage;
    int v_dec = (int)((voltage - v_int) * 100);
    int i_int = (int)Curr[0];
    int i_dec = (int)((Curr[0] - i_int) * 100);
    if (i_dec < 0) i_dec = -i_dec;
    if (v_dec < 0) v_dec = -v_dec;
    
    sprintf(meassage, "[Step %2d] V=%d.%02dV, I=%d.%02dA, SOC=%d%%\r\n", data_count + 1, v_int, v_dec, i_int, i_dec, soc);
    HAL_UART_Transmit(&huart1, (uint8_t*)meassage, strlen(meassage), HAL_MAX_DELAY);
    data_count++;
    inference_counter = data_count;
  }
  
  // 當收集到 50 個樣本時，標準化並準備輸入
  if (data_count >= 50) {
    float* p_input = (float*)data[0];
    
    // 標準化 50 個時間步
    for (int i = 0; i < 50; i++) {
      p_input[i*2 + 0] = (data_buf[i][0] - x_mean[0]) / x_std[0];  // V 歸一化
      p_input[i*2 + 1] = (data_buf[i][1] - x_mean[1]) / x_std[1];  // I 歸一化
    }
    
    return 0;  // 準備好推理
  }
  
  return 1;  // 繼續等待資料
}

int post_process(ai_i8* data[])
{
  /* 取得推理輸出，反標準化，更新 SOC/SOH */
  
  float* p_output = (float*)data[0];
  float raw_soc = p_output[0];
  
  // 反標準化
  float soc_float = (raw_soc * y_std) + y_mean;
  soc = (int)soc_float;
  
  // 範圍限制
  if (soc < 1) soc = 0.0f;
  if (soc > 100) soc =100.0f;
  
  // SOH 簡化處理
  soh = soc;

  // 注意：UI 更新由 RTC 中斷控制（120 秒一次），不在此觸發
  
  sprintf(meassage, "\r\n====== AI INFERENCE RESULT ======\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)meassage, strlen(meassage), HAL_MAX_DELAY);
  
  // 分離 Raw 值的整數與小數部分
  int raw_int = (int)raw_soc;
  int raw_dec = (int)((raw_soc - raw_int) * 100);
  if (raw_dec < 0) raw_dec = -raw_dec;
  
  sprintf(meassage, "[AI] SOC = %d %% (Raw=%d.%02d)\r\n", soc, raw_int, raw_dec);
  HAL_UART_Transmit(&huart1, (uint8_t*)meassage, strlen(meassage), HAL_MAX_DELAY);
  sprintf(meassage, "=================================\r\n\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)meassage, strlen(meassage), HAL_MAX_DELAY);
  
  // 滑動視窗：刪除前 20 個資料
  for (int i = 0; i < 30; i++) {
    data_buf[i][0] = data_buf[i + 20][0];
    data_buf[i][1] = data_buf[i + 20][1];
  }
  data_count = 30;
  inference_counter = data_count;
  
  return 0;
}

/* USER CODE END 2 */

/* Entry points --------------------------------------------------------------*/

void MX_X_CUBE_AI_Init(void)
{
    /* USER CODE BEGIN 5 */
  printf("\r\nTEMPLATE - initialization\r\n");

  ai_boostrap(data_activations0);
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */
  int res = -1;

  if (network) {
    /* 1. 擷取並標準化輸入資料 */
    res = acquire_and_process_data(data_ins);
    
    /* 2. 當資料準備好時執行推理 */
    if (res == 0) {
      res = ai_run();
      
      /* 3. 處理輸出 */
      if (res == 0)
        res = post_process(data_outs);
    }
  }

  if (res == -1) {
    printf("[AI] Process error\r\n");
  }
    /* USER CODE END 6 */
}
#ifdef __cplusplus
}
#endif
