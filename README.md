# 📡 Mesh

> 去中心化 Mesh 网络学习与实验项目 - Meshtastic & MeshCore

---

## 📁 项目结构

```
mesh/
├── Meshtastic/                  # 📡 Meshtastic 项目
│   ├── doc/                     # 📚 学习文档和报告
│   │   ├── README.md            # 基础知识
│   │   ├── 总体学习报告.md       # 完整学习报告
│   │   └── 固件学习报告.md       # 固件开发详解
│   ├── source-code/             # 💻 源代码
│   │   └── meshcore/            # (如有)
│   └── external_flash/          # 🔌 外部刷写工具
│
├── MeshCore/                    # 🔷 MeshCore 项目
│   ├── doc/                     # 📚 研究报告 (9 份)
│   │   ├── 00-index.md          # 索引
│   │   ├── 01-overview.md       # 项目概述
│   │   ├── 02-architecture.md   # 架构分析
│   │   ├── 03-features.md       # 功能特性
│   │   ├── 04-comparison.md     # 与 Meshtastic 对比
│   │   ├── 05-usage.md          # 使用指南
│   │   ├── 06-report.md         # 完整学习报告
│   │   ├── 07-firmware-analysis.md ⭐ 固件源码解析
│   │   └── 08-protocol-spec.md     ⭐ 协议详解
│   └── source-code/             # 💻 官方源码
│       └── meshcore/            # MeshCore 完整源码
│
└── README.md                    # 本文件
```

---

## 🎯 项目目标

1. 学习 Meshtastic 网络原理
2. 学习 MeshCore 架构设计
3. 对比分析两种方案
4. 搭建和配置 Mesh 节点
5. 测试不同场景下的通信距离
6. 探索实际应用场景

---

## 📚 学习资源

### Meshtastic

- **定位**: 大众化离网通信平台
- **特点**: 功能丰富、社区活跃、即插即用
- **文档**: `Meshtastic/doc/`

### MeshCore

- **定位**: 嵌入式轻量级路由协议
- **特点**: 简洁高效、MIT 许可、开发者友好
- **文档**: `MeshCore/doc/` (9 份研究报告)
- **源码**: `MeshCore/source-code/meshcore/`

---

## 🆚 快速对比

| 维度 | Meshtastic | MeshCore |
|------|------------|----------|
| 定位 | 终端用户 | 嵌入式开发者 |
| 许可 | GPL-3.0 | MIT |
| 代码量 | ~50000+ 行 | ~2000 行核心 |
| 内存管理 | 动态分配 | 静态池 |
| 社区 | 大型 | 成长中 |
| 推荐场景 | 个人使用/hobby | 商业/定制开发 |

详细对比见 `MeshCore/doc/04-comparison.md`

---

## 📋 MeshCore 研究文档

| 编号 | 文档 | 内容 |
|------|------|------|
| 01 | overview.md | 项目概述、使命、应用场景 |
| 02 | architecture.md | 架构分析、组件、协议 |
| 03 | features.md | 功能特性、配置、安全 |
| 04 | comparison.md | 与 Meshtastic 详细对比 |
| 05 | usage.md | 使用指南、快速开始 |
| 06 | report.md | 完整学习报告总结 |
| **07** | **firmware-analysis.md** | **固件源码深度解析** ⭐ |
| **08** | **protocol-spec.md** | **协议详解规范** ⭐ |

---

## 🚀 快速开始

### Meshtastic

1. 阅读 `Meshtastic/doc/README.md` 了解基础知识
2. 准备硬件设备（T-Beam/Heltec 等）
3. 刷写固件并配置
4. 开始实验！

### MeshCore

1. 阅读 `MeshCore/doc/01-overview.md` 了解项目
2. 访问 https://flasher.meshcore.co.uk 刷写固件
3. 使用客户端应用连接

### 开发者

1. 查看 `MeshCore/source-code/meshcore/` 源码
2. 阅读 `MeshCore/doc/07-firmware-analysis.md` 了解架构
3. 参考 `MeshCore/doc/08-protocol-spec.md` 了解协议
4. 使用 PlatformIO 编译自定义固件

---

## 📝 待办事项

- [ ] 购买/准备硬件设备
- [ ] 刷写 Meshtastic 固件
- [ ] 刷写 MeshCore 固件
- [ ] 配置节点参数
- [ ] 测试点对点通信
- [ ] 测试 Mesh 中继
- [ ] 记录实验数据
- [ ] 对比两种方案性能

---

_创建时间：2026-03-02_  
_更新时间：2026-03-04_  
_创建者：小 C_
