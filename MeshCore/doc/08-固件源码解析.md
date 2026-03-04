# MeshCore 固件源码解析报告

## 📋 文档信息

- **研究对象**: MeshCore Firmware v1.12.0+
- **源码仓库**: https://github.com/meshcore-dev/MeshCore
- **分析时间**: 2026-03-04
- **分析者**: 小 C

---

## 📁 源码目录结构

```
MeshCore/
├── src/                      # 核心源代码
│   ├── MeshCore.h           # 主头文件
│   ├── Mesh.h / Mesh.cpp    # Mesh 网络层
│   ├── Dispatcher.h / Dispatcher.cpp  # 数据包调度器
│   ├── Packet.h / Packet.cpp  # 数据包结构
│   ├── Identity.h / Identity.cpp  # 身份管理
│   ├── Utils.h / Utils.cpp  # 工具函数
│   └── helpers/             # 辅助模块（9 个子模块）
│
├── include/                  # 公共头文件
├── examples/                 # 示例应用
│   ├── companion_radio/     # 伴随电台（BLE/USB/WiFi）
│   ├── kiss_modem/          # KISS 调制解调器
│   ├── simple_repeater/     # 简单中继器
│   ├── simple_room_server/  # 房间服务器
│   ├── simple_secure_chat/  # 安全聊天
│   └── simple_sensor/       # 传感器节点
│
├── docs/                     # 技术文档
│   ├── packet_format.md     # 数据包格式
│   ├── payloads.md          # 载荷格式
│   ├── companion_protocol.md # 伴随协议
│   ├── kiss_modem_protocol.md # KISS 协议
│   ├── faq.md               # 常见问题
│   └── ...
│
├── arch/                     # 架构相关代码
├── boards/                   # 开发板配置
├── variants/                 # 硬件变体（69 种）
├── lib/                      # 第三方库
├── bin/                      # 预编译二进制
└── platformio.ini            # PlatformIO 配置
```

---

## 🔍 核心模块分析

### 1. Dispatcher（调度器）

**文件**: `src/Dispatcher.h` / `src/Dispatcher.cpp`

**职责**: 底层数据包管理，负责检测传入数据包和队列调度传出数据包

**核心类**:

```cpp
class Dispatcher {
  Radio* _radio;              // 无线电抽象
  MillisecondClock* _ms;      // 毫秒时钟
  PacketManager* _mgr;        // 数据包管理器
  
  void begin();               // 初始化
  void loop();                // 主循环
  
  Packet* obtainNewPacket();  // 获取新数据包
  void releasePacket(Packet* packet);  // 释放数据包
  void sendPacket(Packet* packet, uint8_t priority, uint32_t delay);  // 发送
};
```

**关键特性**:
- 包优先级调度
- 延迟发送支持
- 空中时间预算计算
- CAD（信道活动检测）管理
- 干扰阈值处理

---

### 2. Mesh（网格层）

**文件**: `src/Mesh.h` / `src/Mesh.cpp`

**职责**: 识别特定载荷类型，处理入站和准备出站数据包

**继承关系**:
```
Dispatcher (底层调度)
    ↓
Mesh (网格层)
    ↓
具体实现 (如 CompanionRadio, Repeater 等)
```

**核心方法**:

```cpp
class Mesh : public Dispatcher {
protected:
  // 接收处理
  virtual void onPeerDataRecv(...);      // 对等数据接收
  virtual void onGroupDataRecv(...);     // 群组数据接收
  virtual void onAdvertRecv(...);        // 广告接收
  virtual void onAckRecv(...);           // ACK 接收
  virtual void onPathRecv(...);          // 路径接收
  
  // 路由决策
  virtual bool allowPacketForward(...);  // 是否允许转发
  virtual uint32_t getRetransmitDelay(...);  // 重传延迟
  
  // 发送创建
  Packet* createAdvert(...);             // 创建广告包
  Packet* createDatagram(...);           // 创建数据报
  Packet* createAck(...);                // 创建确认
};
```

---

### 3. Packet（数据包）

**文件**: `src/Packet.h` / `src/Packet.cpp`

**数据结构**:

```cpp
struct Packet {
  uint8_t header;           // 1 字节：版本 + 载荷类型 + 路由类型
  uint8_t transport_codes[4];  // 可选：4 字节传输代码
  uint8_t path_length;      // 1 字节：路径长度
  uint8_t path[MAX_PATH_SIZE];  // 最多 64 字节路径
  uint8_t payload[MAX_PACKET_PAYLOAD];  // 最多 184 字节载荷
  uint8_t len;              // 总长度
};
```

---

## 📦 数据包格式详解

### V1 数据包结构

```
[header][transport_codes(可选)][path_length][path][payload]
```

### Header 字节结构 (1 字节)

```
位：7 6 5 4 3 2 1 0
    │ │ └───┴───┴───┴──┐
    │ │      载荷类型   │
    │ │    (4 位)       │
    │ └───┘ └─┴─┘
    │      版本  路由类型
    │     (2 位)  (2 位)
```

**位掩码**:
- 位 0-1 (`0x03`): 路由类型
- 位 2-5 (`0x3C`): 载荷类型
- 位 6-7 (`0xC0`): 载荷版本

### 路由类型 (2 位)

| 值 | 名称 | 描述 |
|----|------|------|
| `0x00` | ROUTE_TYPE_TRANSPORT_FLOOD | 泛洪路由 + 传输代码 |
| `0x01` | ROUTE_TYPE_FLOOD | 泛洪路由 |
| `0x02` | ROUTE_TYPE_DIRECT | 直接路由 |
| `0x03` | ROUTE_TYPE_TRANSPORT_DIRECT | 直接路由 + 传输代码 |

### 载荷类型 (4 位)

| 值 | 名称 | 描述 |
|----|------|------|
| `0x00` | PAYLOAD_TYPE_REQ | 请求 |
| `0x01` | PAYLOAD_TYPE_RESPONSE | 响应 |
| `0x02` | PAYLOAD_TYPE_TXT_MSG | 文本消息 |
| `0x03` | PAYLOAD_TYPE_ACK | 确认 |
| `0x04` | PAYLOAD_TYPE_ADVERT | 节点广告 |
| `0x07` | PAYLOAD_TYPE_ANON_REQ | 匿名请求 |
| `0x08` | PAYLOAD_TYPE_PATH | 返回路径 |
| `0x09` | PAYLOAD_TYPE_TRACE | 路径追踪 |
| `0x0A` | PAYLOAD_TYPE_MULTIPART | 多部分包 |
| `0x0B` | PAYLOAD_TYPE_CONTROL | 控制数据 |
| `0x0F` | PAYLOAD_TYPE_RAW_CUSTOM | 自定义原始数据 |

---

## 🔐 安全机制

### 加密设计

1. **ECDH 密钥交换**: 使用 Ed25519 公钥/私钥对
2. **共享密钥计算**: 基于对等节点公钥计算共享密钥
3. **MAC 认证**: 2 字节消息认证码
4. **加密载荷**: 使用共享密钥加密数据

### 身份管理

**文件**: `src/Identity.h` / `src/Identity.cpp`

```cpp
struct Identity {
  uint8_t public_key[PUB_KEY_SIZE];  // 32 字节 Ed25519 公钥
};

struct LocalIdentity : public Identity {
  uint8_t secret_key[SECRET_KEY_SIZE];  // 32 字节私钥
};
```

---

## 📡 路由机制

### 泛洪路由 (Flood Routing)

```
节点 A → 广播 → 所有邻居
          ↓
    邻居检查是否已见过此包
          ↓
    未见过 → 转发；见过 → 丢弃
```

**特点**:
- 简单可靠
- 可能产生冗余流量
- 使用包哈希去重

### 直接路由 (Direct Routing)

```
节点 A → [路径：B→C→D] → 节点 D
```

**特点**:
- 需要已知路径
- 更高效
- 路径存储在 packet.path[] 中
- 最多 64 跳

### 零跳路由 (Zero Hop)

```
节点 A → 仅直接邻居
```

**用途**: 本地发现、邻居通信

---

## 🛠️ 示例应用分析

### 1. Companion Radio

**路径**: `examples/companion_radio/`

**功能**: 连接外部聊天应用（BLE/USB/WiFi）

**协议**: 使用 Companion Protocol 与客户端通信

**关键代码**:
```cpp
// BLE 服务 UUID
#define BLE_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
```

### 2. Simple Repeater

**路径**: `examples/simple_repeater/`

**功能**: 消息中继，扩展网络覆盖

**配置**:
- 不连接客户端
- 仅转发数据包
- 低功耗设计

### 3. Simple Sensor

**路径**: `examples/simple_sensor/`

**功能**: 远程传感器节点

**特性**:
- 遥测数据采集
- ACL 访问控制
- 告警通知

---

## 📋 协议文档摘要

### Companion Protocol

**文档**: `docs/companion_protocol.md`

**BLE 特征**:
- **服务 UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX 特征** (App→固件): `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX 特征** (固件→App): `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

**关键命令**:
| 命令 | 代码 | 用途 |
|------|------|------|
| CMD_APP_START | `0x01 0x03` | 初始化通信 |
| CMD_DEVICE_QUERY | `0x16 0x03` | 查询设备信息 |
| CMD_GET_CHANNEL | `0x1F` | 获取频道信息 |
| CMD_SET_CHANNEL | `0x20` | 设置频道 |
| CMD_SEND_CHANNEL_MSG | `0x03` | 发送频道消息 |

### KISS Modem Protocol

**文档**: `docs/kiss_modem_protocol.md`

**用途**: 串口 KISS 协议桥接

**帧格式**:
```
[FEND][命令][数据][FEND]
```

---

## 🔧 构建系统

### PlatformIO 配置

**文件**: `platformio.ini`

**支持平台**:
- ESP32 系列
- nRF52 系列
- STM32 系列

**构建脚本**:
- `build.sh`: 主构建脚本
- `build_as_lib.py`: 库模式构建
- `create-uf2.py`: 创建 UF2 文件

### 硬件支持

**variants/** 目录包含 69 种硬件配置：
- Heltec WiFi LoRa 32 系列
- RAK Wireless 系列
- Seeed Studio 系列
- 其他 ESP32/nRF52 开发板

---

## 💡 设计亮点

### 1. 嵌入式友好

- **无动态内存分配**: 仅在 setup/begin 中分配
- **静态池管理**: PacketManager 使用静态包池
- **确定性行为**: 可预测的内存和时间使用

### 2. 模块化设计

```
应用层 (examples/)
    ↓
网格层 (Mesh)
    ↓
调度层 (Dispatcher)
    ↓
硬件抽象 (Radio, MillisecondClock)
```

### 3. 抽象层设计

**Radio 抽象**:
```cpp
class Radio {
  virtual int recvRaw(uint8_t* bytes, int sz) = 0;
  virtual bool startSendRaw(const uint8_t* bytes, int len) = 0;
  virtual uint32_t getEstAirtimeFor(int len_bytes) = 0;
  // ...
};
```

**优势**: 轻松移植到不同无线电硬件

---

## 📊 性能参数

### 包大小限制

| 字段 | 最大值 | 定义 |
|------|--------|------|
| 路径 | 64 字节 | MAX_PATH_SIZE |
| 载荷 | 184 字节 | MAX_PACKET_PAYLOAD |
| 总包 | ~256 字节 | 取决于配置 |

### 重传机制

- **泛洪模式**: 可配置重传延迟
- **直接模式**: 额外的 ACK 重传
- **优先级调度**: 高优先级包先发送

---

## 🔮 V2 协议规划

根据文档和代码注释，V2 协议可能包括：

1. **更大的哈希**: 2 字节节点哈希（当前 1 字节）
2. **更大的 MAC**: 4 字节 MAC（当前 2 字节）
3. **路径哈希**: 更高效的路径表示
4. **增强的加密规范**: 更安全的加密方案

---

## 📝 学习心得

### MeshCore 的优势

1. **代码简洁**: 核心代码约 2000 行，易于理解
2. **文档完善**: 详细的协议文档和 FAQ
3. **嵌入式优化**: 无动态内存，适合资源受限设备
4. **模块化**: 清晰的分层架构

### 与 Meshtastic 的对比

| 维度 | MeshCore | Meshtastic |
|------|----------|------------|
| 代码量 | ~2000 行核心 | ~50000+ 行 |
| 内存管理 | 静态池 | 动态分配 |
| 学习曲线 | 中等 | 较陡 |
| 可定制性 | 高 | 中等 |

---

## 🔗 参考资源

- **源码**: https://github.com/meshcore-dev/MeshCore
- **协议文档**: `docs/` 目录
- **示例**: `examples/` 目录
- **Discord**: https://discord.gg/BMwCtwHj5V

---

*报告完成时间：2026-03-04*  
*固件版本：v1.12.0*  
*分析者：小 C*
