# ToRadio/FromRadio 协议深度解析

## ❓ 核心问题

> 连接的设备发送 ToRadio 给节点，程序中读出只有 MeshPacket，但是 Config 是怎么通过 MeshPacket 传给节点的？

**答案**：**Config 不是通过 MeshPacket 传输的！而是直接通过 ToRadio/FromRadio 的 `payload_variant` 字段传输的！**

---

## 📦 完整数据结构

### ToRadio 结构 (手机 → 设备)

```protobuf
message ToRadio {
  oneof payload_variant {
    MeshPacket packet = 1;              // ✅ 发送 MeshPacket 到 mesh 网络
    RadioConfig config = 2;             // ❌ 已废弃
    User user = 3;                      // ✅ 设置用户信息
    Position position = 4;              // ✅ 设置位置
    Config config_v2 = 5;               // ✅ 设备配置 (直接传输！)
    ModuleConfig module_config = 6;     // ✅ 模块配置 (直接传输！)
    bytes mqtt_client_proxy_message = 7;
    CoreDeviceUIEvent device_ui_event = 8;
    XModem xmodem = 9;
    Contact contact = 10;
    LocalConfig local_config = 11;
    LocalModuleConfig local_module_config = 12;
    bool disconnect = 14;
    Heartbeat heartbeat = 15;
  }
}
```

### FromRadio 结构 (设备 → 手机)

```protobuf
message FromRadio {
  uint32 id = 1;
  
  oneof payload_variant {
    MyNodeInfo my_info = 2;
    RadioConfig config = 3;             // ❌ 已废弃
    NodeInfo node_info = 4;
    Packet packet = 5;                  // ✅ MeshPacket (从 mesh 网络接收)
    Config config_v2 = 6;               // ✅ 设备配置 (直接传输！)
    ModuleConfig module_config = 7;     // ✅ 模块配置 (直接传输！)
    Channel channel = 8;
    uint32 complete_number = 9;
    LogRecord log_record = 10;
    XModem xmodem = 11;
    DeviceMetadata metadata = 12;
    QueueStatus queue_status = 13;
    MqttClientProxyMessage mqtt_client_proxy_message = 14;
    Contact contact = 15;
    FileInfo file_info = 16;
    ClientNotification notification = 17;
  }
}
```

---

## 🔍 关键区别

### MeshPacket vs Config 传输路径

```
┌─────────────────────────────────────────────────────────────────┐
│                        手机/客户端                               │
└───────────────────────────┬─────────────────────────────────────┘
                            │
        ┌───────────────────┴───────────────────┐
        │                                       │
        ▼                                       ▼
┌───────────────────┐                 ┌───────────────────┐
│   MeshPacket      │                 │   Config          │
│ (应用层数据)       │                 │ (设备配置)         │
│                   │                 │                   │
│ 通过 Mesh 网络     │                 │ 直接 PhoneAPI     │
│ 路由转发          │                 │ 本地处理           │
└─────────┬─────────┘                 └─────────┬─────────┘
          │                                     │
          ▼                                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                     PhoneAPI::handleToRadio()                   │
│                                                                 │
│  switch (toRadioScratch.which_payload_variant) {               │
│    case meshtastic_ToRadio_packet_tag:                         │
│      // MeshPacket → 进入 Mesh 网络                             │
│      return handleToRadioPacket(toRadioScratch.packet);        │
│                                                                │
│    case meshtastic_ToRadio_config_v2_tag:                      │
│      // Config → 直接本地处理，不进入 Mesh 网络！                 │
│      handleSetConfig(toRadioScratch.config_v2);                │
│      break;                                                    │
│                                                                │
│    case meshtastic_ToRadio_module_config_tag:                  │
│      // ModuleConfig → 直接本地处理                             │
│      handleSetModuleConfig(toRadioScratch.module_config);      │
│      break;                                                    │
│  }                                                              │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    设备固件内部处理                               │
│                                                                 │
│  MeshPacket 路径：                                              │
│  PhoneAPI → MeshService → Router → MeshModule → LoRa 发送       │
│                                                                 │
│  Config 路径：                                                  │
│  PhoneAPI → AdminModule → 修改 config 全局变量 → 保存到 Flash     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📝 代码实证分析

### 1. PhoneAPI::handleToRadio() 处理逻辑

**文件**: `src/mesh/PhoneAPI.cpp` (第 147-186 行)

```cpp
bool PhoneAPI::handleToRadio(const uint8_t *buf, size_t bufLength)
{
    // 解析 ToRadio protobuf
    if (pb_decode_from_bytes(buf, bufLength, &meshtastic_ToRadio_msg, &toRadioScratch)) {
        
        switch (toRadioScratch.which_payload_variant) {
        
        // ✅ MeshPacket - 进入 Mesh 网络
        case meshtastic_ToRadio_packet_tag:
            return handleToRadioPacket(toRadioScratch.packet);
            
        // ✅ Config - 直接本地处理
        case meshtastic_ToRadio_config_v2_tag:
            LOG_INFO("Got Config from phone");
            // 直接调用 AdminModule 处理，不进入 Mesh 网络
            adminModule->handleSetConfig(toRadioScratch.config_v2);
            break;
            
        // ✅ ModuleConfig - 直接本地处理
        case meshtastic_ToRadio_module_config_tag:
            LOG_INFO("Got ModuleConfig from phone");
            adminModule->handleSetModuleConfig(toRadioScratch.module_config);
            break;
            
        // ✅ User - 设置用户信息
        case meshtastic_ToRadio_user_tag:
            LOG_INFO("Got User from phone");
            service->reloadOwner(toRadioScratch.user);
            break;
            
        // ✅ Position - 设置位置
        case meshtastic_ToRadio_position_tag:
            LOG_INFO("Got Position from phone");
            // 直接处理位置数据
            break;
            
        default:
            // 忽略其他消息
            break;
        }
    }
    return false;
}
```

### 2. MeshPacket 的处理路径

**文件**: `src/mesh/PhoneAPI.cpp`

```cpp
bool PhoneAPI::handleToRadioPacket(meshtastic_MeshPacket &p)
{
    // MeshPacket 会进入 Mesh 网络
    // 可能被转发到 LoRa、MQTT 或其他节点
    
    LOG_DEBUG("Enqueuing packet to send to mesh");
    
    // 通过 MeshService 发送到 Mesh 网络
    service->sendToMesh(packetPool.allocCopy(p), RX_SRC_USER);
    
    return true;
}
```

**MeshPacket 的旅程**:
```
PhoneAPI.handleToRadioPacket()
    ↓
MeshService.sendToMesh()
    ↓
Router.send()
    ↓
MeshModule.callModules()  ← 所有模块都会处理
    ↓
RadioInterface.send()     ← 通过 LoRa 发送
    ↓
其他节点接收
```

### 3. Config 的处理路径

**文件**: `src/modules/AdminModule.cpp`

```cpp
void AdminModule::handleSetConfig(const meshtastic_Config &c)
{
    // Config 不会进入 Mesh 网络！
    // 直接修改本地配置
    
    switch (c.which_payload_variant) {
    case meshtastic_Config_device_tag:
        config.device = c.payload_variant.device;
        break;
    case meshtastic_Config_lora_tag:
        config.lora = c.payload_variant.lora;
        // 重新配置 LoRa 硬件
        radioInterface->reconfigure();
        break;
    case meshtastic_Config_power_tag:
        config.power = c.payload_variant.power;
        break;
    // ... 其他配置
    }
    
    // 保存到 Flash
    saveChanges(SEGMENT_CONFIG);
}
```

**Config 的旅程**:
```
PhoneAPI.handleToRadio()
    ↓
AdminModule.handleSetConfig()
    ↓
修改 config 全局变量
    ↓
保存到 Flash (saveChanges)
    ↓
重新配置硬件 (reconfigure)
```

**不会**:
- ❌ 通过 LoRa 转发
- ❌ 通过 MQTT 转发
- ❌ 路由到其他节点

---

## 🎯 完整示例

### 示例 1: 手机发送文本消息 (通过 MeshPacket)

```python
# Python 客户端代码
import meshtastic

# 创建 MeshPacket
packet = meshtastic.MeshPacket()
packet.to = 0  # 广播
packet.decoded.portnum = meshtastic.PortNum.TEXT_MESSAGE_APP
packet.decoded.payload = b"Hello!"

# 包装到 ToRadio
to_radio = meshtastic.ToRadio()
to_radio.packet.CopyFrom(packet)  # ✅ 使用 packet 字段

# 发送到设备
client.send_to_radio(to_radio)
```

**设备内部处理**:
```
ToRadio.packet (MeshPacket)
    ↓
PhoneAPI.handleToRadioPacket()
    ↓
MeshService.sendToMesh()
    ↓
Router.send()
    ↓
TextMessageModule.handleReceived()  ← 处理文本消息
    ↓
可能通过 LoRa 转发给其他节点
```

### 示例 2: 手机设置 LoRa 配置 (通过 Config)

```python
# Python 客户端代码
import meshtastic

# 创建 Config
config = meshtastic.Config()
config.lora.modem_preset = meshtastic.ModemPreset.LONG_FAST
config.lora.tx_power = 30

# 包装到 ToRadio
to_radio = meshtastic.ToRadio()
to_radio.config_v2.CopyFrom(config)  # ✅ 使用 config_v2 字段

# 发送到设备
client.send_to_radio(to_radio)
```

**设备内部处理**:
```
ToRadio.config_v2 (Config)
    ↓
PhoneAPI.handleToRadio()
    ↓
AdminModule.handleSetConfig()
    ↓
config.lora = new_config
    ↓
radioInterface.reconfigure()  ← 重新配置 LoRa 硬件
    ↓
saveChanges()  ← 保存到 Flash
```

**不会转发到其他节点！**

### 示例 3: 设备返回配置给手机 (FromRadio)

**文件**: `src/mesh/PhoneAPI.cpp` (第 288-345 行)

```cpp
size_t PhoneAPI::getFromRadio(uint8_t *buf)
{
    switch (state) {
    // ... 其他状态 ...
    
    case STATE_SEND_CONFIG:
        // FromRadio 直接包含 Config，不是 MeshPacket！
        fromRadioScratch.which_payload_variant = 
            meshtastic_FromRadio_config_tag;
        
        switch (config_state) {
        case meshtastic_Config_device_tag:
            fromRadioScratch.config.which_payload_variant = 
                meshtastic_Config_device_tag;
            fromRadioScratch.config.payload_variant.device = 
                config.device;
            break;
        case meshtastic_Config_lora_tag:
            fromRadioScratch.config.which_payload_variant = 
                meshtastic_Config_lora_tag;
            fromRadioScratch.config.payload_variant.lora = 
                config.lora;
            break;
        // ... 其他配置 ...
        }
        break;
        
    case STATE_SEND_MODULECONFIG:
        // ModuleConfig 同理
        fromRadioScratch.which_payload_variant = 
            meshtastic_FromRadio_moduleConfig_tag;
        // ...
        break;
    }
    
    // 编码为 protobuf 字节流
    size_t numbytes = pb_encode_to_bytes(
        buf, 
        meshtastic_FromRadio_size, 
        &meshtastic_FromRadio_msg, 
        &fromRadioScratch
    );
    return numbytes;
}
```

---

## 📊 数据传输对比表

| 数据类型 | ToRadio 字段 | FromRadio 字段 | 是否进入 Mesh 网络 | 是否转发 |
|---------|-------------|----------------|------------------|----------|
| 文本消息 | `packet` | `packet` | ✅ 是 | ✅ 可能 |
| 位置信息 | `packet` (POSITION_APP) | `packet` | ✅ 是 | ✅ 可能 |
| 设备配置 | `config_v2` | `config` | ❌ 否 | ❌ 否 |
| 模块配置 | `module_config` | `moduleConfig` | ❌ 否 | ❌ 否 |
| 用户信息 | `user` | `node_info` | ❌ 否 | ❌ 否 |
| 信道配置 | - | `channel` | ❌ 否 | ❌ 否 |
| 设备元数据 | - | `metadata` | ❌ 否 | ❌ 否 |
| 日志记录 | - | `log_record` | ❌ 否 | ❌ 否 |
| MQTT 代理 | `mqttClientProxyMessage` | `mqttClientProxyMessage` | ❌ 否 | ❌ 否 |

---

## 🔑 关键要点

### 1. **两种传输路径**

```
路径 A: MeshPacket (应用数据)
ToRadio.packet → Mesh 网络 → 可能通过 LoRa 转发 → 其他节点

路径 B: Config/ModuleConfig (设备配置)
ToRadio.config_v2 → AdminModule → 本地配置 → 不转发
```

### 2. **为什么这样设计？**

- **MeshPacket**: 需要在 mesh 网络中路由，可能被多个节点接收和转发
- **Config**: 只影响本地设备，不应该转发给其他节点

### 3. **Data.payload 的用途**

你提到的 `meshtastic_Data_payload_t payload` 字段是用于 **MeshPacket 内部的应用数据**：

```protobuf
message MeshPacket {
  Data decoded = 12;  // 解码后的数据
}

message Data {
  PortNum portnum = 1;    // 应用类型
  bytes payload = 2;      // ✅ 这里是应用层数据
                          // 例如：文本消息的字符串
                          //       位置信息的经纬度
                          //       遥测数据的传感器读数
}
```

**示例**:
```python
# 文本消息的 payload
packet.decoded.portnum = TEXT_MESSAGE_APP
packet.decoded.payload = b"Hello Meshtastic!"  # 这里是实际消息内容

# 位置信息的 payload
packet.decoded.portnum = POSITION_APP
packet.decoded.payload = position.SerializeToString()  # 序列化后的位置数据

# 遥测数据的 payload
packet.decoded.portnum = TELEMETRY_APP
packet.decoded.payload = telemetry.SerializeToString()  # 传感器数据
```

---

## 🎨 完整数据流图

```
┌─────────────────────────────────────────────────────────────────────┐
│                              手机客户端                              │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ 发送文本消息  │  │ 设置设备配置  │  │ 请求节点信息  │              │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │
│         │                 │                 │                        │
│         ▼                 ▼                 ▼                        │
│  ┌─────────────────────────────────────────────────────────┐        │
│  │              ToRadio (protobuf)                         │        │
│  │  ┌─────────────┬─────────────┬─────────────────────┐   │        │
│  │  │ packet      │ config_v2   │ (其他字段...)       │   │        │
│  │  │ (MeshPacket)│ (Config)    │                     │   │        │
│  │  └─────────────┴─────────────┴─────────────────────┘   │        │
│  └─────────────────────────────────────────────────────────┘        │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ BLE/WiFi/Serial 传输
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        设备固件 (PhoneAPI)                          │
│                                                                     │
│  pb_decode_from_bytes() 解析 ToRadio                               │
│                                                                     │
│  switch (toRadio.which_payload_variant) {                          │
│                                                                     │
│    case packet_tag:        // ✅ MeshPacket                        │
│      handleToRadioPacket()                                          │
│        ↓                                                            │
│      MeshService.sendToMesh()                                       │
│        ↓                                                            │
│      Router.send()                                                  │
│        ↓                                                            │
│      MeshModule.callModules()  ← 所有模块处理                       │
│        ↓                                                            │
│      RadioInterface.send()  ← 通过 LoRa 发送                        │
│                                                                     │
│    case config_v2_tag:     // ✅ Config                            │
│      AdminModule.handleSetConfig()                                  │
│        ↓                                                            │
│      config.device = new_config                                     │
│        ↓                                                            │
│      saveChanges()  ← 保存到 Flash                                  │
│        ↓                                                            │
│      radioInterface.reconfigure()  ← 重新配置硬件                   │
│                                                                     │
│    case module_config_tag: // ✅ ModuleConfig                      │
│      AdminModule.handleSetModuleConfig()                            │
│        ↓                                                            │
│      moduleConfig.mqtt = new_config                                 │
│        ↓                                                            │
│      saveChanges()                                                  │
│  }                                                                  │
└───────────────────────────┬─────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          响应返回给手机                              │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────┐        │
│  │              FromRadio (protobuf)                       │        │
│  │  ┌─────────┬──────────┬──────────┬──────────────┐      │        │
│  │  │ packet  │ config   │ node_info│ metadata     │ ...  │        │
│  │  │ (应用)  │ (配置)   │ (节点)   │ (元数据)     │      │        │
│  │  └─────────┴──────────┴──────────┴──────────────┘      │        │
│  └─────────────────────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 📚 参考资料

- [PhoneAPI.cpp 源码](https://github.com/meshtastic/firmware/blob/master/src/mesh/PhoneAPI.cpp)
- [AdminModule.cpp 源码](https://github.com/meshtastic/firmware/blob/master/src/modules/AdminModule.cpp)
- [protobuf 定义](https://buf.build/meshtastic/protobufs)
- [ToRadio 定义](https://buf.build/meshtastic/protobufs/docs/main:meshtastic#meshtastic.ToRadio)
- [FromRadio 定义](https://buf.build/meshtastic/protobufs/docs/main:meshtastic#meshtastic.FromRadio)

---
更新时间：2026-03-05
