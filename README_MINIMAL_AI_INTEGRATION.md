# X-CUBE-AI SOC 最小改动融合方案

## 📌 设计理念：最小改动法

**目标**: 保持CUBEMx生成代码的完整性，所有AI逻辑完全隔离到app_x-cube-ai.c中。

---

## 📂 文件改动总结

### ✅ Core/Src/main.c (保持原生)
```c
仅改动:
  1行: 在主循环最后添加 MX_X_CUBE_AI_Process();
  
不改动:
  ✓ 所有CUBEMx自动生成部分保持完整
  ✓ Update_AdcReading() 继续工作(读取ADC)
  ✓ 所有peripheral初始化保持原样
  
删除:
  ✗ 移除所有AI相关定义(缓冲、标准化函数等)
  ✗ 恢复RTC回调到原始状态(只设置epd_update_flag)
```

### ✅ X-CUBE-AI/App/app_x-cube-ai.c (整合所有AI逻辑)
```c
修改:
  USER CODE 2 区间:
    - 添加标准化参数常量 (从JSON获取)
    - 实现 acquire_and_process_data() - 采集/缓冲/标准化
    - 实现 post_process() - 反标准化/输出
  
  USER CODE 6 区间:
    - 改写 MX_X_CUBE_AI_Process() 为单次推理
    
不改动:
  ✓ ai_log_err(), ai_boostrap(), ai_run() 保持原样
  ✓ 框架级代码完全不动
```

### ✅ X-CUBE-AI/App/app_x-cube-ai.h (最小化)
```c
改动:
  - 删除 ai_run_inference() 函数声明
  
结果:
  - 只保留 MX_X_CUBE_AI_Init() 和 MX_X_CUBE_AI_Process()
```

---

## 🔄 数据流 (简洁版)

```
[RTC中断 每120秒]
    ↓ 设置 epd_update_flag=1
    ↓
[main循环]
    ├─ Update_AdcReading() 【CUBEMx生成】
    │   └─ 更新全局 voltage
    ├─ 读取GPIO状态
    │   └─ 更新全局 Curr[0]
    ├─ MX_X_CUBE_AI_Process() 【新增调用】
    │   ├─ acquire_and_process_data()
    │   │   ├─ 采集 voltage, Curr[0]
    │   │   ├─ 填充缓冲[data_buf]
    │   │   └─ 满20个样本时标准化
    │   ├─ ai_run() 【CUBEMx框架】
    │   │   └─ 执行X-CUBE-AI推理
    │   └─ post_process()
    │       ├─ 反标准化输出
    │       ├─ 更新 soc/soh
    │       └─ 设置 epd_update_flag=1
    └─ UI显示更新 (if epd_update_flag)
```

---

## 📊 代码对比

### 之前 (复杂方案) ❌
- main.c: +120行 AI定义和函数
- 缓冲区、标准化、推理全在main.c
- 难以维护，混入CUBEMx代码

### 之后 (最小改动法) ✅
- main.c: +1行 (调用MX_X_CUBE_AI_Process)
- app_x-cube-ai.c: 完整实现AI流程
- CUBEMx代码完全隔离，易于维护

---

## 🎯 核心逻辑

### 标准化参数 (内嵌在app_x-cube-ai.c)
```c
// 输入归一化
x_normalized = (x_raw - x_mean) / x_std

// 输出反标准化
soc = raw_output * y_std + y_mean

// 参数来自JSON:
x_mean = {3.625, -1.964}
x_std = {0.262, 0.265}
y_mean = 52.535
y_std = 28.375
```

### 滑动窗口
```c
// 满20个样本后推理
if (inference_counter >= 20) {
  perform_inference();  // 使用全30个样本
  
  // 删除前20个，保留后10个
  for (i = 0; i < 10; i++) {
    data_buf[i] = data_buf[i+20];
  }
  data_count = 10;
  inference_counter = 0;
}
```

---

## 📦 编译结果

```
Build Status: ✅ SUCCESS (无warning)
RAM Usage: 27632 B / 128 KB (21.08%)
FLASH Usage: 135592 B / 512 KB (25.86%)
```

---

## 🔍 工作机制

### 外部接口 (main.c中的全局变量)
app_x-cube-ai.c通过extern声明访问:
```c
extern float voltage, Curr[20];      // 输入: 电压和电流
extern int soc, soh;                 // 输出: SOC/SOH
extern uint8_t epd_update_flag;      // 标志: UI更新
```

### 内部状态 (app_x-cube-ai.c私有)
```c
static float data_buf[30][2];        // 30个时间步的缓冲
static uint16_t data_count = 0;      // 当前样本数
static uint16_t inference_counter;   // 推理触发计数
```

---

## ⚙️ 调整点

### RTC采样频率
**当前**: 120秒 (太稀疏)  
**建议**: 1秒  
```c
// 在 MX_RTC_Init() 中修改
- HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 120, ...);
+ HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 1, ...);
```

### 推理阈值
**当前**: 20个样本  
**改为**: 30个样本使用完整缓冲
```c
static const uint16_t INFERENCE_THRESHOLD = 30;  // 等待缓冲满
```

---

## 💡 优势

| 方面 | 收获 |
|------|------|
| **代码可读性** | CUBEMx部分一目了然，AI逻辑集中 |
| **维护性** | 新改CUBEMx配置时只需重新生成，无需重写AI |
| **内存占用** | 相同 (~600B for AI数据结构) |
| **易于调试** | 所有AI函数都在一个文件中 |

---

## 🚀 使用流程

1. **烧写固件** → 项目编译完毕
2. **监控UART** → 查看AI Inference打印
3. **观察电子纸** → SOC值显示更新
4. **调整参数** → 根据效果修改阈值/频率

---

## 📋 核心函数说明

### acquire_and_process_data()
```c
// 返回0: 数据已准备好，可以推理
// 返回1: 继续等待更多数据
```

**功能**:
- 从voltage/Curr[0]采集数据
- 填充缓冲区
- 当满20个样本时，标准化并填充AI输入

### post_process()
```c
// 返回0: 成功处理
// 返回-1: 错误
```

**功能**:
- 获取AI输出
- 反标准化为实际SOC值
- 更新全局soc/soh
- 触发UI更新

---

## 🔧 关键改动对照

### main.c
```c
// 之前: 无AI调用
// 之后: 在主循环末添加
    MX_X_CUBE_AI_Process();
```

### app_x-cube-ai.c
```c
// 之前: acquire_and_process_data() 和 post_process() 为空
// 之后: 完整实现数据采集、缓冲、推理、输出处理
```

---

## 📞 快速检查表

- [ ] 编译无error且无warning
- [ ] 烧写后UART能看到初始化消息
- [ ] 20秒后看到 "[AI] SOC=..." 打印
- [ ] 电子纸显示SOC值
- [ ] 再等20秒，SOC更新第二次

---

## ✨ 特点总结

✅ **最小改动** - main.c仅1行改动  
✅ **易读易维护** - 所有AI逻辑集中在一个文件  
✅ **标准化流程** - 遵循X-CUBE-AI框架结构  
✅ **可扩展** - 未来修改模型时无需改main.c  
✅ **编译干净** - 无warning，编译优化  

---

**状态**: ✅ 就绪烧写  
**编译时间**: <5秒  
**二进制大小**: 135.6 KB
