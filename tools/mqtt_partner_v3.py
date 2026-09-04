#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""mqtt_partner_v3.py
  专为当前 GUI 里实际订阅主题定制：
      GUI 订阅 = debug/platform/+/data/#
  所以发送到 = debug/platform/devA/data       (3 层：匹配 + 3 层)
           = debug/platform/devA/data/5     (4 层：匹配 + # 多层)

  5 组共 16 条（case ③ data/5 也命中，因为末尾 #）
  用法： py -3 mqtt_partner_v3.py
"""
from __future__ import annotations

import json
import os
import random
import sys
import time
from datetime import datetime

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("[FATAL] 缺少 paho-mqtt。请先运行： py -3 -m pip install paho-mqtt")
    sys.exit(1)


BROKER = "broker.emqx.io"
PORT = 1883
CLIENT_ID = f"PartnerPy3_{int(time.time())}"
KEEPALIVE = 30

# 用户 GUI 里 MQTT_DevA 的订阅 = debug/platform/+/data/#
# 因此我们的 ShortId = devA 填入 + 那一层。
PUB_TOPIC      = "debug/platform/devA/data"
PUB_TOPIC_N    = "debug/platform/devA/data/5"    # addr=5 走路径型（末尾 /<addr>）
REPLY_TOPIC    = "debug/platform/devA/reply"
CMD_SUB_TOPIC  = "debug/platform/DebugPlatform/cmd"


def now() -> str:
    return datetime.now().strftime("%H:%M:%S")


def main() -> int:
    print(f"[{now()}] ============ MQTT 联调搭档 v3（匹配 GUI 实际订阅） 启动 ============")
    print(f"ClientId={CLIENT_ID}")

    connected = {"rc": None}
    received: list[dict] = []

    def on_connect(c, userdata, flags, rc):
        connected["rc"] = rc
        print(f"[{now()}] Python 连接 broker {BROKER}:{PORT}  rc={rc}")
        if rc == 0:
            c.subscribe([
                (CMD_SUB_TOPIC, 0),
                (REPLY_TOPIC, 0),
            ])
            print(f"  订阅: {CMD_SUB_TOPIC} 和 {REPLY_TOPIC}")
        else:
            print(f"  !! 连接失败，rc={rc}")

    def on_message(c, userdata, msg):
        try:
            payload = msg.payload.decode("utf-8", errors="replace")
        except Exception:
            payload = repr(msg.payload)
        received.append({"ts": time.time(), "topic": msg.topic, "payload": payload})
        print(f"[{now()}] ← RX topic={msg.topic} qos={msg.qos} payload={payload[:200]}")

    c = mqtt.Client(client_id=CLIENT_ID, clean_session=True)
    c.on_connect = on_connect
    c.on_message = on_message
    try:
        c.connect(BROKER, PORT, KEEPALIVE)
    except Exception as e:
        print(f"[{now()}] 连接异常: {e}")
        return 2
    c.loop_start()

    # 等待 CONNACK
    deadline = time.time() + 5.0
    while connected["rc"] is None and time.time() < deadline:
        time.sleep(0.2)
    if connected["rc"] != 0:
        print(f"[{now()}] 5s 内未收到 CONNACK，退出")
        c.loop_stop()
        c.disconnect()
        return 1

    time.sleep(1.0)  # 再多等一下，确保 GUI 侧订阅也到位

    # ========== 发送 5 组消息（与 v2 完全同构，仅 topic 前缀变） ==========
    print()
    # ① 单点 JSON —— addr 0
    msg1 = {"addr": 0, "value": 12.34, "deviceId": "devA", "name": "测试点0"}
    payload1 = json.dumps(msg1, ensure_ascii=False)
    print(f"[{now()}] ① 单点 JSON -> {PUB_TOPIC} = {payload1}")
    c.publish(PUB_TOPIC, payload1, qos=0, retain=False)
    time.sleep(1.0)

    # ② 数组 JSON —— addr 10/11/12
    msg2 = {"data": [
        {"addr": 10, "value": 1.11, "name": "测点10"},
        {"addr": 11, "value": 2.22, "name": "测点11"},
        {"addr": 12, "value": 3.33, "name": "测点12"},
    ]}
    payload2 = json.dumps(msg2, ensure_ascii=False)
    print(f"[{now()}] ② 数组 JSON -> {PUB_TOPIC} = {payload2}")
    c.publish(PUB_TOPIC, payload2, qos=0, retain=False)
    time.sleep(1.0)

    # ③ 纯数值（路径型）—— addr=5，value=88.88
    #    因为 GUI 订阅结尾带 #，debug/platform/devA/data/5 会命中 → MqttProtocol.topicPathAddress 分支
    payload3 = "88.88"
    print(f"[{now()}] ③ 纯数值(路径) -> {PUB_TOPIC_N} = {payload3}")
    c.publish(PUB_TOPIC_N, payload3, qos=0, retain=False)
    time.sleep(1.0)

    # ④ 高频连发 10 条 —— addr=20..29，值随机 20.00~29.99
    print(f"[{now()}] ④ 高频连发 10 条 -> {PUB_TOPIC} addr=20..29")
    for i in range(10):
        addr = 20 + i
        val = round(20.0 + random.random() * 10.0, 4)
        m = {"addr": addr, "value": val, "deviceId": "devA"}
        c.publish(PUB_TOPIC, json.dumps(m, ensure_ascii=False), qos=0, retain=False)
        time.sleep(0.05)  # 略微间隔，避免瞬时淹没 broker
    time.sleep(1.0)

    # ⑤ 异常值 —— addr=99, error=high_limit_exceeded
    msg5 = {"addr": 99, "value": -9999.99, "error": "high_limit_exceeded",
            "message": "超过上阈值，触发紧急回退"}
    payload5 = json.dumps(msg5, ensure_ascii=False)
    print(f"[{now()}] ⑤ 异常值 -> {PUB_TOPIC} addr=99 error=high_limit_exceeded")
    c.publish(PUB_TOPIC, payload5, qos=0, retain=False)

    print(f"[{now()}] ============ 所有消息发送完毕，继续监听 15 秒 ============")
    time.sleep(15.0)

    c.loop_stop()
    c.disconnect()
    print(f"[{now()}] 搭档退出")
    return 0


if __name__ == "__main__":
    sys.exit(main())
