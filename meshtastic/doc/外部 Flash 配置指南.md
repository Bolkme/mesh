# 外部 SPI Flash 配置指南

## 📌 概述

本文档说明如何在 STM32WL 平台上使用外部 SPI Flash 存储配置信息，以扩展存储空间并保护内部 Flash。

## 🎯 为什么使用外部 Flash？

1. **扩展存储容量** - 内部 Flash 有限，外部 Flash 可提供 2MB-128MB+ 空间
2. **保护内部 Flash** - 减少内部 Flash 擦写次数，延长芯片寿命
3. **配置持久化** - 独立存储配置文件，固件升级不丢失配置
4. **灵活性** - 可根据需求选择不同容量的 Flash 芯片

## 🔧 硬件连接

### 推荐 Flash 芯片

| 型号 | 容量 | 电压 | 封装 | 备注 |
|------|------|------|------|------|
| W25Q64JVSIQ | 8MB | 2.7-3.6V | SOP-8 | 推荐 |
| W25Q128JVSIQ | 16MB | 2.7-3.6V | SOP-8 | 大容量 |
| MX25L6406E | 8MB | 2.7-3.6V | SOP-8 | 替代方案 |
| W25Q32JVSIQ | 4MB | 2.7-3.6V | SOP-8 | 经济方案 |

### 引脚连接

| Flash 引脚 | STM32WL 引脚 | 说明 |
|-----------|-------------|------|
| VCC | 3.3V | 电源 |
| GND | GND | 地 |
| CS | PA4 (可配置) | 片选 |
| CLK | PA5 (SPI_SCK) | 时钟 |
| DI/MOSI | PA7 (SPI_MOSI) | 数据输入 |
| DO/MISO | PA6 (SPI_MISO) | 数据输出 |
| WP/HOLD | NC 或上拉 | 写保护/保持 |

### 电路图示例

```
          W25Q64JVSIQ
          ┌─────────┐
    3.3V ─┤ VCC   1 ││ 8 ── CS ──── PA4
          │         ││
          │         ││
    MOSI ─┤ DI    2 ││ 7 ── DO ──── MISO (PA6)
    GND ──┤ GND   3 ││ 6 ── WP ──── NC (或上拉)
          │         ││
          │         ││
    SCK ──┤ CLK   4 ││ 5 ── HOLD ── NC (或上拉)
          └─────────┘
```

## 💻 软件配置

### 1. 启用外部 Flash

在 `platformio.ini` 或 `variant.h` 中定义：

```ini
[env:your_board]
build_flags =
    -D USE_EXTERNAL_FLASH
    -D EXTERNAL_FLASH_CS=PA4
    -D EXTERNAL_FLASH_SIZE_BYTES=8388608  ; 8MB
```

### 2. 在代码中使用

#### 方式 A：直接使用 ExternalFS

```cpp
#include "platform/stm32wl/ExternalFlashFS.h"

void setup() {
    // 初始化外部 Flash 文件系统
    if (!ExternalFS.begin()) {
        LOG_ERROR("ExternalFS mount failed");
        return;
    }
    
    LOG_INFO("ExternalFS mounted successfully");
    LOG_INFO("Flash size: %u bytes", ExternalFS.getFlashSize());
}

void saveConfig(const char *filename, const uint8_t *data, size_t size) {
    File f = ExternalFS.open(filename, FILE_O_WRITE);
    if (f) {
        f.write(data, size);
        f.flush();
        f.close();
        LOG_INFO("Config saved to %s", filename);
    }
}

void loadConfig(const char *filename, uint8_t *buffer, size_t size) {
    File f = ExternalFS.open(filename, FILE_O_READ);
    if (f) {
        f.read(buffer, size);
        f.close();
        LOG_INFO("Config loaded from %s", filename);
    }
}
```

#### 方式 B：修改 FSCommon.h 切换文件系统

在 `FSCommon.h` 中添加外部 Flash 支持：

```cpp
#if defined(ARCH_STM32WL) && defined(USE_EXTERNAL_FLASH)
// STM32WL with External Flash
#include "ExternalFlashFS.h"
#define FSCom ExternalFS
#define FSBegin() ExternalFS.begin()
#else
// STM32WL with Internal Flash
#include "LittleFS.h"
#define FSCom InternalFS
#define FSBegin() FSCom.begin()
using namespace STM32_LittleFS_Namespace;
#endif
```

### 3. 配置选项

可以在 `ExternalFlashFS.h` 中修改以下配置：

```cpp
// 引脚配置
#define EXTERNAL_FLASH_CS PA4

// Flash 参数（根据芯片规格调整）
#define EXTERNAL_FLASH_SIZE_BYTES (8 * 1024 * 1024)  // 8MB
#define EXTERNAL_FLASH_BLOCK_SIZE 4096
#define EXTERNAL_FLASH_PAGE_SIZE 256
#define EXTERNAL_FLASH_SECTOR_SIZE 4096

// LittleFS 参数
#define EXTERNAL_FLASH_LFS_READ_SIZE 256
#define EXTERNAL_FLASH_LFS_PROG_SIZE 256
#define EXTERNAL_FLASH_LFS_BLOCK_SIZE EXTERNAL_FLASH_BLOCK_SIZE
#define EXTERNAL_FLASH_LFS_BLOCK_COUNT (EXTERNAL_FLASH_SIZE_BYTES / EXTERNAL_FLASH_BLOCK_SIZE)
#define EXTERNAL_FLASH_LFS_LOOKAHEAD 32
```

## 📁 文件组织建议

```
外部 Flash 存储结构：
/
├── /config/           # 配置文件
│   ├── device.conf    # 设备配置
│   ├── mesh.conf      # Mesh 网络配置
│   └── user.conf      # 用户配置
├── /logs/             # 日志文件
│   └── system.log
├── /firmware/         # 固件备份（可选）
│   └── backup.bin
└── /data/             # 应用数据
    └── ...
```

## 🔍 调试和诊断

### 检查 Flash 连接

```cpp
void checkFlashHealth() {
    uint32_t jedecId = ExternalFS.getFlashId();
    LOG_INFO("Flash JEDEC ID: 0x%06X", jedecId);
    
    if (jedecId == 0 || jedecId == 0xFFFFFF) {
        LOG_ERROR("Flash chip not detected!");
        return;
    }
    
    LOG_INFO("Flash size: %u bytes", ExternalFS.getFlashSize());
    
    // 列出文件
    ExternalFS.begin();
    listDir("/", 2);
}
```

### 常见问题

#### 1. Flash 未检测到

**症状**: `getFlashId()` 返回 0 或 0xFFFFFF

**解决方案**:
- 检查电源连接（3.3V 稳定）
- 检查 SPI 引脚连接
- 检查 CS 引脚配置
- 测量 SPI 信号质量

#### 2. 挂载失败

**症状**: `ExternalFS.begin()` 返回 false

**解决方案**:
- 尝试格式化：`ExternalFS.format()`
- 检查 Flash 容量配置是否正确
- 检查 LittleFS 参数是否匹配 Flash 规格

#### 3. 读写错误

**症状**: 文件操作返回错误或数据损坏

**解决方案**:
- 检查 SPI 频率（降低到 5MHz 测试）
- 确保在操作前获取 SPI 锁
- 检查电源稳定性

## 🚀 完整示例

```cpp
/**
 * @file external_flash_example.cpp
 * @brief External SPI Flash usage example for Meshtastic STM32WL
 */

#include "ExternalFlashFS.h"
#include "FSCommon.h"
#include "configuration.h"

// 配置结构示例
struct DeviceConfig {
    char deviceName[32];
    uint32_t frequency;
    int8_t txPower;
    uint8_t region;
    uint32_t magic;  // 用于验证配置有效性
};

#define CONFIG_MAGIC 0xMESHTASTIC
#define CONFIG_FILE "/config/device.conf"

DeviceConfig defaultConfig = {
    .deviceName = "Meshtastic_Node",
    .frequency = 915000000,
    .txPower = 22,
    .region = 1,
    .magic = CONFIG_MAGIC
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    LOG_INFO("=== External Flash Example ===");
    
    // 初始化外部 Flash
    if (!ExternalFS.begin()) {
        LOG_ERROR("ExternalFS initialization failed!");
        // 可以回退到内部 Flash
        // InternalFS.begin();
        return;
    }
    
    LOG_INFO("ExternalFS initialized");
    LOG_INFO("Flash ID: 0x%06X", ExternalFS.getFlashId());
    LOG_INFO("Flash Size: %u bytes", ExternalFS.getFlashSize());
    
    // 列出文件
    LOG_INFO("Files in root directory:");
    listDir("/", 2);
    
    // 测试配置读写
    testConfigReadWrite();
}

void loop() {
    // 定期保存数据示例
    delay(60000);
    saveSensorData();
}

void testConfigReadWrite() {
    // 写入配置
    File f = ExternalFS.open(CONFIG_FILE, FILE_O_WRITE);
    if (f) {
        f.write((uint8_t*)&defaultConfig, sizeof(DeviceConfig));
        f.flush();
        f.close();
        LOG_INFO("Config written to %s", CONFIG_FILE);
    }
    
    // 读取配置
    DeviceConfig loadedConfig;
    f = ExternalFS.open(CONFIG_FILE, FILE_O_READ);
    if (f) {
        f.read((uint8_t*)&loadedConfig, sizeof(DeviceConfig));
        f.close();
        
        if (loadedConfig.magic == CONFIG_MAGIC) {
            LOG_INFO("Config loaded successfully:");
            LOG_INFO("  Device Name: %s", loadedConfig.deviceName);
            LOG_INFO("  Frequency: %lu Hz", loadedConfig.frequency);
            LOG_INFO("  TX Power: %d dBm", loadedConfig.txPower);
        } else {
            LOG_ERROR("Config magic mismatch!");
        }
    }
}

void saveSensorData() {
    // 示例：保存传感器数据到外部 Flash
    static uint32_t counter = 0;
    counter++;
    
    char filename[64];
    snprintf(filename, sizeof(filename), "/data/sensor_%lu.csv", counter);
    
    File f = ExternalFS.open(filename, FILE_O_WRITE);
    if (f) {
        f.print("timestamp,temperature,humidity\n");
        f.print(millis());
        f.print(",");
        f.print(25.5);  // 示例数据
        f.print(",");
        f.println(60);
        f.close();
        LOG_DEBUG("Sensor data saved: %s", filename);
    }
}
```

## 📝 更新日志

- **2026-03-03**: 初始版本，添加 ExternalFlashFS 支持

## 🔗 相关资源

- [LittleFS 文档](https://github.com/littlefs-project/littlefs)
- [W25Q64 数据手册](https://www.winbond.com/resource-files/w25q64jv_dtr%20revc%2003272018%20plus.pdf)
- [STM32WL 参考手册](https://www.st.com/resource/en/reference_manual/rm0453-stm32wl-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
