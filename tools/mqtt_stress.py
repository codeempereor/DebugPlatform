#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""mqtt_stress.py
  高频压力测试搭档 → 验证 DebugPlatform 的数据层在高并发下的稳定性。
  策略：尽可能快地发 N 条 MQTT 消息到 debug/platform/devA/data，
        覆盖 DataCache / 协议解析 / SQLite 批量入库 / 日志面板 全链路。

  用法: py -3 mqtt_stress.py [总条数] [间隔秒]
  默认: py -3 mqtt_stress.py 1000 0.0   (1000 条，零间隔 → 最快)

  建议分三档跑:
    ① 轻压   500 条  0.0s  → 基本不丢点
    ② 中压  1000 条  0.0s  → 考验 SQLite 批量写入
    ③ 重压  3000 条  0.0s  → 极端场景，看 UI 是否卡死
"""
from __future__ import annotations

import json
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
CLIENT_ID = f"StressPy_{int(time.time())}"
KEEPALIVE = 30

# 与 GUI 订阅 debug/platform/+/data/# 匹配
PUB_TOPIC = "debug/platform/devA/data"
REPLY_TOPIC = "debug/platform/devA/reply"


def now() -> str:
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


def main() -> int:
    total = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
    interval = float(sys.argv[2]) if len(sys.argv) > 2 else 0.0

    print(f"[{now()}] ============ MQTT 压力测试启动 ============ ")
    print(f"  目标 Broker: {BROKER}:{PORT}")
    print(f"  发送主题:   {PUB_TOPIC}")
    print(f"  消息总数:   {total} 条")
    print(f"  消息间隔:   {interval}s  ({'最快' if interval == 0 else f'{interval*1000:.0f}ms'})")
    print(f"  预计耗时:   ~{total * max(interval, 0.001):.1f}s")

    connected = {"rc": None}
    received_count = {"n": 0}
    received_topics = set()

    def on_connect(c, userdata, flags, rc):
        connected["rc"] = rc
        print(f"[{now()}] CONNACK rc={rc}")
        if rc == 0:
            c.subscribe([
                (REPLY_TOPIC, 0),
            ])

    def on_message(c, userdata, msg):
        received_count["n"] += 1
        received_topics.add(msg.topic)
        if received_count["n"] <= 5:
            print(f"[{now()}] ← RX #{received_count['n']} topic={msg.topic} payload={msg.payload[:120].decode('utf-8', errors='replace')}")

    c = mqtt.Client(client_id=CLIENT_ID, clean_session=True)
    c.on_connect = on_connect
    c.on_message = on_message

    try:
        c.connect(BROKER, PORT, KEEPALIVE)
    except Exception as e:
        print(f"[{now()}] 连接异常: {e}")
        return 2
    c.loop_start()

    # 等 CONNACK
    deadline = time.time() + 5.0
    while connected["rc"] is None and time.time() < deadline:
        time.sleep(0.1)
    if connected["rc"] != 0:
        print(f"[{now()}] 5s 内未连接成功")
        c.loop_stop()
        c.disconnect()
        return 1

    time.sleep(0.5)  # 确保 GUI 订阅到位

    # ========== 开始高压发送 ==========
    print(f"\n[{now()}] >>> 开始发送 {total} 条消息 <<<\n")
    t_start = time.time()
    sent = 0
    lost = 0

    for i in range(total):
        # 轮询地址：0-99 循环，覆盖多个寄存器
        addr = i % 100

        # 40% 概率发数组格式（3点），60% 概率发单点
        if i % 5 == 0:
            # 数组格式: addr, addr+1, addr+2
            data = [
                {"addr": addr,     "value": round(random.uniform(0, 100), 4), "deviceId": "devA", "name": f"压测点{addr}"},
                {"addr": addr + 1, "value": round(random.uniform(0, 100), 4), "deviceId": "devA"},
                {"addr": addr + 2, "value": round(random.uniform(0, 100), 4), "deviceId": "devA"},
            ]
            payload = json.dumps({"data": data}, ensure_ascii=False)
            points = 3
        else:
            # 单点格式
            msg = {
                "addr": addr,
                "value": round(random.uniform(-1000, 1000), 4),
                "deviceId": "devA",
            }
            payload = json.dumps(msg, ensure_ascii=False)
            points = 1

        result = c.publish(PUB_TOPIC, payload, qos=0, retain=False)
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            sent += points
        else:
            lost += 1

        # 进度提示
        if (i + 1) % 200 == 0 or (i + 1) == total:
            elapsed = time.time() - t_start
            rate = (i + 1) / elapsed if elapsed > 0 else 0
            eta = (total - i - 1) * interval + elapsed if interval > 0 else 0
            print(f"[{now()}] 进度 {i+1}/{total} ({(i+1)*100//total}%)  "
                  f"已发 {sent} 个寄存器点  "
                  f"速度 {rate:.0f} msg/s  "
                  f"丢包 {lost}  "
                  f"已耗时 {elapsed:.1f}s  "
                  f"剩余 {eta:.1f}s")

        if interval > 0:
            time.sleep(interval)

    t_end = time.time()
    elapsed = t_end - t_start

    # ========== 统计 ==========
    total_points = total * 3  # 最多 3 点/条（数组），但实际约 2 点/条平均
    print(f"\n[{now()}] ============ 发送完毕 ============ ")
    print(f"  消息条数:   {total} 条")
    print(f"  覆盖寄存器: {sent} 个点 (数组消息展开后)")
    print(f"  丢包:       {lost} 条")
    print(f"  总耗时:     {elapsed:.2f}s")
    print(f"  平均速度:   {total/elapsed if elapsed > 0 else 0:.0f} msg/s")
    print(f"  收到回复:   {received_count['n']} 条 (topic: {received_topics or '无'})")
    print(f"\n  ★ 请在 GUI 端核对:")
    print(f"    1) 数据监控面板寄存器值是否全部刷新（特别是高频地址）")
    print(f"    2) 日志面板 [protocol] 解析行数 ≈ {total} 行或更多")
    print(f"    3) 日志面板 [data] MQTT WRITE 行数 ≈ {sent} 行")
    print(f"    4) GUI 是否有卡顿/冻结现象")
    print(f"    5) 状态栏 '告警' 是否为 0（压测不应触发告警）")

    # 等 5 秒看回复
    print(f"\n[{now()}] 等待 5 秒看 GUI 是否回复 ...")
    time.sleep(5.0)

    c.loop_stop()
    c.disconnect()
    print(f"[{now()}] 搭档退出。再见！")
    return 0


if __name__ == "__main__":
    sys.exit(main())
