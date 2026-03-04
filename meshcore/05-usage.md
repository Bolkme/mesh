# MeshCore 使用指南

## 🚀 快速开始

### 方式一：刷写预编译固件（推荐新手）

```
步骤 1: 访问固件刷写工具
        https://flasher.meshcore.co.uk

步骤 2: 选择你的设备
        - Heltec WiFi LoRa 32
        - RAK Wireless
        - 其他支持设备

步骤 3: 选择固件类型
        - Companion（连接手机/电脑）
        - Repeater（中继器）
        - Room Server（BBS 服务器）

步骤 4: 刷写固件
        使用 Adafruit ESPTool 或类似工具

步骤 5: 连接客户端
        - Web: https://app.meshcore.nz
        - Android: Google Play
        - iOS: App Store
```

### 方式二：开发者编译（推荐开发者）

```bash
# 1. 安装 PlatformIO (VSCode 扩展)
# https://docs.platformio.org

# 2. 克隆仓库
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore

# 3. 在 VSCode 中打开项目

# 4. 选择示例应用
# - examples/companion_radio
# - examples/simple_repeater
# - examples/simple_sensor
# 等...

# 5. 修改代码（按需）

# 6. 编译并上传
# 使用 PlatformIO 工具栏
```

---

## 📱 客户端配置

### Web 客户端

1. 访问 https://app.meshcore.nz
2. 连接 Companion 设备（BLE/USB/WiFi）
3. 开始聊天

### Android 客户端

1. Google Play 下载 "MeshCore"
2. 打开应用
3. 通过 BLE 或 WiFi 连接设备
4. 配置节点名称和设置

### iOS 客户端

1. App Store 下载 "MeshCore"
2. 打开应用
3. 通过 BLE 连接设备
4. 开始使用

---

## 🔧 设备配置

### USB 串口配置

1. 访问 https://config.meshcore.dev
2. 通过 USB 连接设备
3. 配置以下参数：
   - 节点名称
   - 区域设置
   - 射频参数
   - 角色设置

### 远程管理（通过 LoRa）

1. 打开移动应用
2. 连接到 Companion 节点
3. 使用"远程管理"功能
4. 配置网络中的 Repeater/Room Server

---

## 📡 典型部署场景

### 场景 1: 两人户外通信

```
用户 A                      用户 B
  │                          │
  ▼                          ▼
┌─────────┐              ┌─────────┐
│Companion│◄────────────►│Companion│
│  Node   │   直连 LoRa   │  Node   │
└─────────┘              └─────────┘
  │                          │
  ▼                          ▼
手机 App                    手机 App

距离：~5-10km（视距）
```

### 场景 2: 扩展覆盖范围（使用中继）

```
用户 A                      用户 B
  │                          │
  ▼                          ▼
┌─────────┐    ┌─────────┐  ┌─────────┐
│Companion│◄──►│Repeater │◄─►│Companion│
│  Node   │    │  Node   │  │  Node   │
└─────────┘    └─────────┘  └─────────┘
  │              ▲              │
  ▼              │              ▼
手机 App      高处部署        手机 App

距离：~15-30km（通过中继）
```

### 场景 3: 传感器网络

```
┌─────────┐    ┌─────────┐    ┌─────────┐
│ Sensor  │◄──►│Repeater │◄─►│Companion│
│  Node   │    │  Node   │    │  Node   │
└─────────┘    └─────────┘    └─────────┘
     │                           │
     │  传感器数据               │  数据收集
     ▼                           ▼
  环境监测                    手机/电脑
```

---

## 📋 示例应用说明

### 1. Companion Radio

**用途**: 连接外部聊天应用

**连接方式**:
- BLE（蓝牙低功耗）
- USB 串口
- WiFi

**配置**:
```cpp
// 在 examples/companion_radio 中配置
#define USE_BLE true
#define USE_WIFI false
#define USE_USB true
```

### 2. KISS Modem

**用途**: 串口 KISS 协议桥接

**协议文档**: `docs/kiss_modem_protocol.md`

**适用**: 主机应用集成

### 3. Simple Repeater

**用途**: 消息中继，扩展覆盖

**部署建议**:
- 放置在制高点
- 供电稳定（太阳能 + 电池）
- 配置为不连接客户端

### 4. Simple Room Server

**用途**: BBS 公告板

**功能**:
- 共享帖子
- 公共消息
- 离线留言

### 5. Simple Secure Chat

**用途**: 安全终端通信

**使用方式**:
- VSCode 串口监视器
- Android 串口终端

### 6. Simple Sensor

**用途**: 远程传感器

**功能**:
- 遥测数据采集
- 告警通知
- ACL 访问控制

---

## 🔍 故障排查

### 问题：无法连接设备

**检查**:
- [ ] 设备已正确刷写固件
- [ ] USB 连接正常
- [ ] 蓝牙已启用（如使用 BLE）
- [ ] 客户端应用是最新版本

### 问题：消息无法发送

**检查**:
- [ ] 节点角色配置正确
- [ ] 射频参数匹配（频率、带宽）
- [ ] 在 LoRa 覆盖范围内
- [ ] 中继节点工作正常

### 问题：网络覆盖不足

**解决**:
- 添加 Repeater 节点
- 提高天线位置
- 使用更高增益天线
- 调整射频参数

---

## 📚 学习资源

| 资源 | 链接 |
|------|------|
| 官方文档 | https://github.com/meshcore-dev/MeshCore |
| FAQ | `/docs/faq.md` |
| KISS 协议 | `/docs/kiss_modem_protocol.md` |
| 固件刷写 | https://flasher.meshcore.co.uk |
| 配置工具 | https://config.meshcore.dev |
| Discord 社区 | https://discord.gg/BMwCtwHj5V |
| 介绍视频 | https://youtu.be/t1qne8uJBAc |

---

## 💡 最佳实践

1. **节点命名**: 使用有意义的名称（如"Base-Home"、"Repeater-Mountain"）
2. **中继部署**: 放置在制高点，确保视距
3. **电源管理**: 远程节点使用太阳能 + 电池
4. **频率合规**: 遵守当地无线电法规
5. **安全配置**: 启用加密，配置 ACL（如适用）
6. **定期维护**: 检查远程节点状态

---

*研究创建时间：2026-03-04*
