# External SPI Flash Module for Meshtastic STM32WL

## 📦 文件说明

本模块为 STM32WL 平台提供外部 SPI Flash 文件系统支持，可独立于 Meshtastic Firmware 使用。

### 核心文件

| 文件 | 说明 |
|------|------|
| `ExternalFlashFS.h` | 头文件 - 定义 ExternalFlashFS 类和 API |
| `ExternalFlashFS.cpp` | 实现文件 - SPI Flash 驱动和 LittleFS 集成 |

### 使用方法

1. **复制文件到项目**
   ```bash
   cp external_flash/ExternalFlashFS.* your_project/src/
   ```

2. **在 platformio.ini 中启用**
   ```ini
   build_flags = 
       -D USE_EXTERNAL_FLASH
       -D EXTERNAL_FLASH_CS=PA4
       -D EXTERNAL_FLASH_SIZE_BYTES=8388608
   ```

3. **在代码中包含**
   ```cpp
   #include "ExternalFlashFS.h"
   
   void setup() {
       ExternalFS.begin();
       // 使用 ExternalFS 进行文件操作
   }
   ```

## 🔧 配置选项

在 `platformio.ini` 或 `variant.h` 中配置：

```ini
; 引脚配置
-D EXTERNAL_FLASH_CS=PA4

; Flash 芯片参数
-D EXTERNAL_FLASH_SIZE_BYTES=8388608    ; 8MB
-D EXTERNAL_FLASH_BLOCK_SIZE=4096
-D EXTERNAL_FLASH_PAGE_SIZE=256

; LittleFS 参数
-D EXTERNAL_FLASH_LFS_READ_SIZE=256
-D EXTERNAL_FLASH_LFS_PROG_SIZE=256
-D EXTERNAL_FLASH_LFS_LOOKAHEAD=32
```

## 📚 完整文档

详细使用指南请参考：`../doc/外部 Flash 配置指南.md`

## 💡 示例代码

```cpp
#include "ExternalFlashFS.h"

void setup() {
    if (!ExternalFS.begin()) {
        Serial.println("ExternalFS mount failed!");
        return;
    }
    
    // 保存配置
    File f = ExternalFS.open("/config.dat", "w");
    f.print("Hello External Flash!");
    f.close();
    
    // 读取配置
    f = ExternalFS.open("/config.dat", "r");
    String data = f.readString();
    Serial.println(data);
    f.close();
}
```

## 📝 更新日志

- **2026-03-03**: 初始版本
  - 支持 W25Q/MX25 系列 SPI Flash
  - LittleFS 文件系统集成
  - 自动容量检测
  - 深度功耗模式

## 🔗 相关资源

- [外部 Flash 配置指南（中文）](../doc/外部%20Flash%20配置指南.md)
- [LittleFS 项目](https://github.com/littlefs-project/littlefs)
- [W25Q64 数据手册](https://www.winbond.com/resource-files/w25q64jv_dtr%20revc%2003272018%20plus.pdf)
