# 📡 Mesh

> 去中心化 Mesh 网络学习与实验项目 - Meshtastic & MeshCore

---

## 📁 项目结构

```
mesh/
├── meshtastic/               # 📡 Meshtastic 项目
│   ├── doc/                  # 📚 学习笔记和文档
│   │   ├── README.md         # Meshtastic 基础知识
│   │   ├── 总体学习报告.md    # 完整学习报告
│   │   └── 固件学习报告.md    # 固件开发详解
│   ├── external_flash/       # 🔌 外部刷写工具
│   └── source-code/          # 💻 源代码
│
├── meshcore/                 # 🔷 MeshCore 研究报告
│   ├── 00-index.md           # 索引和资源链接
│   ├── 01-overview.md        # 项目概述
│   ├── 02-architecture.md    # 架构分析
│   ├── 03-features.md        # 功能特性
│   ├── 04-comparison.md      # 与 Meshtastic 对比
│   ├── 05-usage.md           # 使用指南
│   └── 06-report.md          # 完整学习报告
│
└── README.md                 # 本文件
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
- **文档**: `meshtastic/doc/`

### MeshCore

- **定位**: 嵌入式轻量级路由协议
- **特点**: 简洁高效、MIT 许可、开发者友好
- **文档**: `meshcore/06-report.md`

---

## 🆚 快速对比

| 维度 | Meshtastic | MeshCore |
|------|------------|----------|
| 定位 | 终端用户 | 嵌入式开发者 |
| 许可 | GPL-3.0 | MIT |
| 复杂度 | 中 | 低 |
| 社区 | 大型 | 成长中 |
| 推荐场景 | 个人使用/hobby | 商业/定制开发 |

详细对比见 `meshcore/04-comparison.md`

---

## 🚀 快速开始

### Meshtastic

1. 阅读 `meshtastic/doc/README.md` 了解基础知识
2. 准备硬件设备（T-Beam/Heltec 等）
3. 刷写固件并配置
4. 开始实验！

### MeshCore

1. 阅读 `meshcore/01-overview.md` 了解项目
2. 访问 https://flasher.meshcore.co.uk 刷写固件
3. 使用客户端应用连接

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
