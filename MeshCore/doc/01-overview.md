# MeshCore 项目概述

## 🌐 项目简介

**MeshCore** 是一个多平台系统，用于实现基于 LoRa 无线电硬件的安全文本通信。它是一个轻量级、可移植的 C++ 库，支持多跳数据包路由，专为嵌入式项目设计。

### 核心定位

> "连接人和物，无需使用互联网"

MeshCore 专注于为离线和离网通信提供安全、可靠、高效的去中心化网状网络系统。

---

## 🎯 项目使命

创建最可靠、最安全的去中心化网状无线电网络系统，供任何人用于通信。**核心关注点：安全性、可靠性、效率**

---

## 📦 技术特点

| 特性 | 描述 |
|------|------|
| **多跳路由** | 设备可通过中间节点转发消息，扩展单无线电覆盖范围 |
| **去中心化** | 无需中央服务器或互联网，网络自愈 |
| **低功耗** | 适合电池或太阳能供电设备 |
| **轻量级** | 专为嵌入式系统设计，代码简洁 |
| **无动态内存分配** | 仅在 setup/begin 函数中分配内存 |
| **多平台支持** | 支持 Heltec、RAK Wireless 等 LoRa 硬件 |

---

## 👥 节点角色

MeshCore 使用**固定角色**设计：

| 角色 | 功能 |
|------|------|
| **Companion（伴随节点）** | 不转发消息，连接外部聊天应用（BLE/USB/WiFi） |
| **Repeater（中继器）** | 扩展网络覆盖范围，转发消息 |
| **Room Server（房间服务器）** | 简单的 BBS 服务器，用于共享帖子 |
| **Sensor（传感器）** | 远程传感器节点，带遥测和告警功能 |

---

## 🛠️ 应用场景

1. **离网通信** - 偏远地区保持连接
2. **应急响应 & 灾难恢复** - 基础设施瘫痪时快速组网
3. **户外活动** - 徒步、露营、探险通信
4. **战术 & 安全应用** - 军事、执法、私人安保
5. **IoT & 传感器网络** - 远程传感器数据收集

---

## 📱 客户端支持

| 平台 | 链接 |
|------|------|
| **Web** | https://app.meshcore.nz |
| **Android** | Google Play Store |
| **iOS** | App Store |
| **NodeJS** | https://github.com/liamcottle/meshcore.js |
| **Python CLI** | https://github.com/fdlamotte/meshcore-cli |

---

## 🔗 官方资源

- **官网**: https://meshcore.co.uk/
- **GitHub**: https://github.com/meshcore-dev/MeshCore
- **固件刷写工具**: https://flasher.meshcore.co.uk
- **配置工具**: https://config.meshcore.dev
- **Discord 社区**: https://discord.gg/BMwCtwHj5V
- **介绍视频**: https://www.youtube.com/watch?v=t1qne8uJBAc

---

*研究创建时间：2026-03-04*
