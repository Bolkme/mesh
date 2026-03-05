# Web 界面与串口配置传输详解

## 🎯 核心问题

> config.pb.h 中的配置内容，从 Web 网页界面接串口发送给节点进行配置，这一部分是如何实现的？配置字段是从何而来？

**答案**: **Web 界面和串口都使用 ToRadio/FromRadio 协议！config.pb.h 的配置结构直接放在 ToRadio.config_v2 字段中传输！**

---

## 📡 完整传输路径

### 路径 1: Web 界面 → WiFi → 节点

```
┌─────────────────────────────────────────────────────────────────┐
│                    Web 浏览器界面                                 │
│  https://192.168.4.1/admin                                     │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  配置表单 (LoRa 设置、设备名称等)                          │   │
│  │  ┌─────────────────────────────────────────────────┐    │   │
│  │  │ Region: UNSET                                   │    │   │
│  │  │ Modem Preset: LONG_FAST                         │    │   │
│  │  │ TX Power: 30 dBm                                │    │   │
│  │  └─────────────────────────────────────────────────┘    │   │
│  │                                                           │   │
│  │  [保存配置] 按钮                                          │   │
│  └─────────────────────────────────────────────────────────┘   │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            │ 用户点击"保存配置"
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    JavaScript (Web 界面)                         │
│                                                                 │
│  // 创建 Config protobuf 对象                                   │
│  const config = {                                               │
│    lora: {                                                      │
│      region: 0,         // UNSET                                │
│      modemPreset: 3,    // LONG_FAST                            │
│      txPower: 30                                                │
│    }                                                            │
│  };                                                             │
│                                                                 │
│  // 序列化为 protobuf 二进制                                    │
│  const configBytes = Config.encode(config).finish();            │
│                                                                 │
│  // 包装到 ToRadio                                              │
│  const toRadio = {                                              │
│    configV2: configBytes  // ✅ 直接放这里！                     │
│  };                                                             │
│                                                                 │
│  // 发送到设备                                                  │
│  fetch('/api/v1/toradio', {                                     │
│    method: 'PUT',                                               │
│    body: ToRadio.encode(toRadio).finish()                       │
│  });                                                            │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            │ HTTP PUT /api/v1/toradio
                            │ Content-Type: application/x-protobuf
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                  设备固件 (ESP32 WebServer)                      │
│                                                                 │
│  文件：src/mesh/http/ContentHandler.cpp                        │
│  函数：handleAPIv1ToRadio()                                    │
│                                                                 │
│  void handleAPIv1ToRadio(HTTPRequest *req, HTTPResponse *res) {│
│    // 1. 读取 HTTP PUT 请求的 protobuf 数据                       │
│    byte buffer[MAX_TO_FROM_RADIO_SIZE];                        │
│    size_t s = req->readBytes(buffer, MAX_TO_FROM_RADIO_SIZE);  │
│                                                                 │
│    // 2. 调用 PhoneAPI 处理 ToRadio                            │
│    webAPI.handleToRadio(buffer, s);                            │
│  }                                                              │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    PhoneAPI::handleToRadio()                    │
│                                                                 │
│  文件：src/mesh/PhoneAPI.cpp                                   │
│                                                                 │
│  bool PhoneAPI::handleToRadio(const uint8_t *buf,              │
│                               size_t bufLength) {              │
│                                                                 │
│    // 1. 解析 ToRadio protobuf                                 │
│    pb_decode_from_bytes(buf, bufLength,                        │
│                         &meshtastic_ToRadio_msg,               │
│                         &toRadioScratch);                      │
│                                                                 │
│    // 2. 根据 payload_variant 分发处理                         │
│    switch (toRadioScratch.which_payload_variant) {             │
│                                                                 │
│      case meshtastic_ToRadio_config_v2_tag:                    │
│        // ✅ Config 配置！                                      │
│        LOG_INFO("Got Config from phone");                      │
│        adminModule->handleSetConfig(                           │
│          toRadioScratch.config_v2);                            │
│        break;                                                  │
│                                                                 │
│      case meshtastic_ToRadio_packet_tag:                       │
│        // MeshPacket - 进入 Mesh 网络                           │
│        handleToRadioPacket(toRadioScratch.packet);             │
│        break;                                                  │
│    }                                                            │
│  }                                                              │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                  AdminModule::handleSetConfig()                 │
│                                                                 │
│  文件：src/modules/AdminModule.cpp                             │
│                                                                 │
│  void AdminModule::handleSetConfig(const Config &c) {          │
│    // 1. 根据配置类型应用配置                                   │
│    switch (c.which_payload_variant) {                          │
│      case meshtastic_Config_lora_tag:                          │
│        config.lora = c.payload_variant.lora;                   │
│        // ✅ 修改全局 config 变量                                │
│        break;                                                  │
│      case meshtastic_Config_device_tag:                        │
│        config.device = c.payload_variant.device;               │
│        break;                                                  │
│      // ... 其他配置类型                                       │
│    }                                                            │
│                                                                 │
│    // 2. 保存到 Flash                                           │
│    saveChanges(SEGMENT_CONFIG);                                │
│                                                                 │
│    // 3. 重新配置硬件                                           │
│    radioInterface->reconfigure();                              │
│  }                                                              │
└─────────────────────────────────────────────────────────────────┘
```

### 路径 2: 串口 → 节点

```
┌─────────────────────────────────────────────────────────────────┐
│                    串口客户端 (Python/CLI)                       │
│                                                                 │
│  import meshtastic                                              │
│  import serial                                                  │
│                                                                 │
│  # 打开串口                                                     │
│  ser = serial.Serial('/dev/ttyUSB0', 115200)                   │
│                                                                 │
│  # 创建 Config                                                  │
│  config = meshtastic.Config()                                   │
│  config.lora.region = 0                                         │
│  config.lora.modem_preset = 3                                   │
│                                                                 │
│  # 包装到 ToRadio                                               │
│  to_radio = meshtastic.ToRadio()                                │
│  to_radio.config_v2.CopyFrom(config)  # ✅ 直接复制             │
│                                                                 │
│  # 序列化为 protobuf                                            │
│  data = to_radio.SerializeToString()                            │
│                                                                 │
│  # 添加串口帧格式 (起始符 + 长度 + 数据 + 结束符)                │
│  frame = b'\x94' + len(data).to_bytes(2, 'big') + data + b'\x93'│
│                                                                 │
│  # 发送到设备                                                   │
│  ser.write(frame)                                               │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            │ 串口 UART (115200 波特率)
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                  设备固件 (SerialConsole)                        │
│                                                                 │
│  文件：src/SerialConsole.cpp                                   │
│  继承：SerialConsole → StreamAPI → PhoneAPI                    │
│                                                                 │
│  bool SerialConsole::handleToRadio(const uint8_t *buf,         │
│                                    size_t len) {               │
│    // 1. 检查串口是否启用                                       │
│    if (config.has_lora && config.security.serial_enabled) {    │
│      // 2. 调用父类 StreamAPI::handleToRadio()                 │
│      return StreamAPI::handleToRadio(buf, len);                │
│    }                                                            │
│  }                                                              │
│                                                                 │
│  文件：src/mesh/StreamAPI.cpp                                  │
│  bool StreamAPI::handleToRadio(const uint8_t *buf,             │
│                                size_t len) {                   │
│    // 3. 调用 PhoneAPI::handleToRadio() - 与 Web 相同！         │
│    return PhoneAPI::handleToRadio(buf, len);                   │
│  }                                                              │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
              后续处理与 Web 界面完全相同！
              PhoneAPI → AdminModule → 保存配置
```

---

## 🔍 config.pb.h 的配置结构

### config.pb.h 定义示例

**文件**: `src/mesh/generated/meshtastic/config.pb.h`

```cpp
/* 配置结构定义 */
typedef struct _meshtastic_Config {
  pb_size_t which_payload_variant;
  union {
    meshtastic_Config_DeviceConfig device;
    meshtastic_Config_PositionConfig position;
    meshtastic_Config_PowerConfig power;
    meshtastic_Config_NetworkConfig network;
    meshtastic_Config_DisplayConfig display;
    meshtastic_Config_LoRaConfig lora;      // ← LoRa 配置
    meshtastic_Config_BluetoothConfig bluetooth;
    meshtastic_Config_SecurityConfig security;
  } payload_variant;
} meshtastic_Config;

/* LoRa 配置详细结构 */
typedef struct _meshtastic_Config_LoRaConfig {
  meshtastic_Config_LoRaConfig_RegionCode region;
  meshtastic_Config_LoRaConfig_ModemPreset modem_preset;
  uint32_t bandwidth;
  uint32_t spread_factor;
  uint8_t coding_rate;
  int8_t tx_power;
  uint32_t frequency_offset;
  // ... 更多字段
} meshtastic_Config_LoRaConfig;
```

### ToRadio 中使用 Config

**文件**: `protobufs/meshtastic/mesh.proto`

```protobuf
message ToRadio {
  oneof payload_variant {
    MeshPacket packet = 1;              // MeshPacket
    Config config_v2 = 5;               // ✅ Config 配置！
    ModuleConfig module_config = 6;     // ✅ 模块配置！
    // ... 其他字段
  }
}
```

---

## 📝 完整代码示例

### 示例 1: Web 界面 JavaScript

```javascript
// Web 界面代码 (简化版)
async function saveLoRaConfig() {
  // 1. 创建 Config 对象
  const config = {
    lora: {
      region: 0,              // UNSET
      modemPreset: 3,         // LONG_FAST
      txPower: 30,            // 30 dBm
      bandwidth: 0,           // 125 kHz
      spreadFactor: 10,       // SF10
      codingRate: 5           // 4/5
    }
  };

  // 2. 序列化为 protobuf
  const configBytes = meshtastic_Config.encode(config).finish();

  // 3. 包装到 ToRadio
  const toRadio = {
    configV2: configBytes  // ✅ 直接放在 config_v2 字段
  };

  // 4. 发送到设备
  const response = await fetch('/api/v1/toradio', {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/x-protobuf'
    },
    body: meshtastic_ToRadio.encode(toRadio).finish()
  });

  console.log('配置已保存！');
}
```

### 示例 2: Python 串口配置

```python
import meshtastic
import serial
from meshtastic.protobuf import mesh_pb2, config_pb2

# 1. 打开串口
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# 2. 创建 Config
config = config_pb2.Config()
config.lora.region = 0  # UNSET
config.lora.modem_preset = 3  # LONG_FAST
config.lora.tx_power = 30

# 3. 包装到 ToRadio
to_radio = mesh_pb2.ToRadio()
to_radio.config_v2.CopyFrom(config)  # ✅ 直接复制

# 4. 序列化
data = to_radio.SerializeToString()

# 5. 添加串口帧格式
frame = b'\x94' + len(data).to_bytes(2, 'big') + data + b'\x93'

# 6. 发送
ser.write(frame)
print('配置已发送！')

# 7. 读取响应 (FromRadio)
response = ser.read(1024)
from_radio = mesh_pb2.FromRadio()
from_radio.ParseFromString(response)

if from_radio.HasField('config'):
    print('配置已应用！')
```

### 示例 3: Meshtastic CLI 工具

```bash
# 使用官方 CLI 工具设置配置
meshtastic --port /dev/ttyUSB0 --set lora.region UNSET
meshtastic --port /dev/ttyUSB0 --set lora.modem_preset LONG_FAST
meshtastic --port /dev/ttyUSB0 --set lora.tx_power 30

# CLI 内部实现 (Python):
# 1. 创建 Config
# 2. 包装到 ToRadio.config_v2
# 3. 通过串口发送
# 4. 等待 FromRadio 确认
```

---

## 🔄 配置保存流程

### 1. 接收配置

```cpp
// PhoneAPI::handleToRadio()
case meshtastic_ToRadio_config_v2_tag:
  adminModule->handleSetConfig(toRadioScratch.config_v2);
  break;
```

### 2. 应用配置

```cpp
// AdminModule::handleSetConfig()
void AdminModule::handleSetConfig(const Config &c) {
  switch (c.which_payload_variant) {
    case meshtastic_Config_lora_tag:
      // ✅ 修改全局 config 变量
      config.lora = c.payload_variant.lora;
      
      // ✅ 重新配置 LoRa 硬件
      radioInterface->reconfigure();
      break;
      
    case meshtastic_Config_device_tag:
      config.device = c.payload_variant.device;
      break;
      
    // ... 其他配置
  }
  
  // ✅ 保存到 Flash
  saveChanges(SEGMENT_CONFIG);
}
```

### 3. 保存到 Flash

```cpp
// AdminModule::saveChanges()
void AdminModule::saveChanges(int saveWhat, bool shouldReboot) {
  if (saveWhat & SEGMENT_CONFIG) {
    // 保存 config 到 Flash
    nodeDB->saveToDisk(SEGMENT_CONFIG);
  }
  
  if (shouldReboot) {
    // 重启设备使配置生效
    rebootAtMsec = millis() + 5000;
  }
}
```

---

## 📊 配置字段来源

### config.pb.h 的生成流程

```
protobufs/meshtastic/config.proto  ← 原始定义
         ↓
protoc 编译器
         ↓
src/mesh/generated/meshtastic/config.pb.h  ← 生成的 C 头文件
src/mesh/generated/meshtastic/config.pb.c  ← 生成的 C 源文件
         ↓
编译到固件中
         ↓
设备可以使用 Config 结构
```

### 配置字段映射

| Web 界面字段 | Config 结构 | ToRadio 字段 |
|-------------|-----------|-------------|
| LoRa Region | `config.lora.region` | `ToRadio.config_v2.lora.region` |
| Modem Preset | `config.lora.modem_preset` | `ToRadio.config_v2.lora.modem_preset` |
| TX Power | `config.lora.tx_power` | `ToRadio.config_v2.lora.tx_power` |
| Device Name | `config.device.nickname` | `ToRadio.config_v2.device.nickname` |
| Bluetooth Enabled | `config.bluetooth.enabled` | `ToRadio.config_v2.bluetooth.enabled` |

---

## 🎨 完整数据流图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         配置源 (Config Source)                       │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  Web 界面     │  │  Python CLI  │  │  Android App │              │
│  │  (JavaScript)│  │  (串口)      │  │  (BLE)       │              │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │
│         │                 │                 │                        │
│         └─────────────────┴─────────────────┘                        │
│                           │                                          │
│                           ▼                                          │
│              ┌────────────────────────┐                             │
│              │   ToRadio.config_v2    │  ← Config 放在这里！         │
│              │   (protobuf 二进制)     │                             │
│              └────────────────────────┘                             │
└───────────────────────────┬─────────────────────────────────────────┘
                            │
                            │ WiFi/串口/BLE 传输
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      设备固件 (Device Firmware)                      │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  PhoneAPI::handleToRadio()                                  │   │
│  │                                                             │   │
│  │  pb_decode_from_bytes()  // 解析 ToRadio                   │   │
│  │                                                             │   │
│  │  switch (which_payload_variant) {                          │   │
│  │    case config_v2_tag:                                     │   │
│  │      // ✅ 配置数据！                                       │   │
│  │      adminModule->handleSetConfig();                       │   │
│  │      break;                                                │   │
│  │  }                                                          │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                            │                                        │
│                            ▼                                        │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  AdminModule::handleSetConfig()                             │   │
│  │                                                             │   │
│  │  config.lora = new_config.lora;  // 修改全局变量            │   │
│  │  radioInterface->reconfigure();   // 重新配置硬件           │   │
│  │  saveChanges(SEGMENT_CONFIG);     // 保存到 Flash           │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                            │                                        │
│                            ▼                                        │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  配置生效                                                   │   │
│  │  - LoRa 频率改变                                             │   │
│  │  - 设备名称更新                                             │   │
│  │  - 蓝牙开关切换                                             │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 🔑 关键要点

### 1. **Config 不是通过 MeshPacket 传输**

```cpp
// ❌ 错误理解
ToRadio.packet.decoded.payload = config_data;  // 错误！

// ✅ 正确方式
ToRadio.config_v2 = config_data;  // 直接放在 config_v2 字段
```

### 2. **ToRadio 是外层容器**

```protobuf
ToRadio {
  payload_variant: {
    oneof {
      packet        // MeshPacket (应用数据)
      config_v2     // Config (设备配置) ← 配置放这里！
      module_config // ModuleConfig (模块配置)
      user          // User (用户信息)
      ...
    }
  }
}
```

### 3. **config.pb.h 的配置结构直接使用**

```cpp
// config.pb.h 中定义的结构可以直接使用
meshtastic_Config_LoRaConfig lora_config;
lora_config.region = 0;
lora_config.modem_preset = 3;
lora_config.tx_power = 30;

// 直接复制到 ToRadio.config_v2
to_radio.config_v2.payload_variant.lora = lora_config;
```

### 4. **Web 界面和串口使用相同的协议**

- Web 界面：HTTP PUT → PhoneAPI
- 串口：UART → StreamAPI → PhoneAPI
- BLE：GATT WRITE → PhoneAPI

**最终都调用同一个 `PhoneAPI::handleToRadio()` 函数！**

---

## 📚 参考资源

- [Web 界面源码](https://github.com/meshtastic/firmware/blob/master/src/mesh/http/ContentHandler.cpp)
- [PhoneAPI 源码](https://github.com/meshtastic/firmware/blob/master/src/mesh/PhoneAPI.cpp)
- [AdminModule 源码](https://github.com/meshtastic/firmware/blob/master/src/modules/AdminModule.cpp)
- [Protobuf 定义](https://buf.build/meshtastic/protobufs)
- [Web API 文档](https://meshtastic.org/docs/development/device/http-api/)

---
更新时间：2026-03-05
