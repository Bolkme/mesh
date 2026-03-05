# MSproto - Meshtastic Protocol Research

研究其他设备与 Meshtastic 节点之间通过 protobuf 数据的连接方式

## 📋 研究目标

1. **外部设备通信协议**
   - 手机 App ↔ Meshtastic 节点（蓝牙/WiFi）
   - 第三方设备 ↔ Meshtastic 节点（串口/UDP）
   - MQTT 服务器 ↔ Meshtastic 节点

2. **protobuf 数据结构分析**
   - ToRadio / FromRadio 消息格式
   - MeshPacket 结构详解
   - PortNum 端口号定义

3. **集成方案**
   - Python 客户端实现
   - REST API 封装
   - WebSocket 桥接

## 📚 参考资料

- [Meshtastic Protobuf 定义](https://github.com/meshtastic/protobufs)
- [Meshtastic API 文档](https://meshtastic.org/docs/software/api/)
- [Python 库](https://github.com/meshtastic/meshtastic-python)

## 🔍 待研究内容

- [ ] PhoneAPI 协议分析
- [ ] 蓝牙 GATT 服务特征
- [ ] WiFi API 接口
- [ ] MQTT 主题和消息格式
- [ ] 串口协议实现
- [ ] 第三方设备集成案例

---
创建时间：2026-03-05
