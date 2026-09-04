# -*- coding: utf-8 -*-
"""MQTT 联调搭档：公共 broker (broker.emqx.io)
paho-mqtt 2.x 兼容写法
"""
import paho.mqtt.client as mqtt
import time, json, sys, os

BROKER = "broker.emqx.io"
PORT   = 1883
LOG = os.path.join(r"d:\TraeWork\data\DebugPlatform\screenshots", "mqtt_partner.log")

def log(s):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(s + "\n")
    print(s)

def on_connect(c, u, f, rc, props=None):
    log(f"[{time.strftime('%H:%M:%S')}] Python 连接 broker {BROKER}:{PORT}  rc={rc}")
    c.subscribe("debug/platform/DebugPlatform/cmd", qos=0)
    c.subscribe("debug/platform/+/reply", qos=0)
    log(f"  订阅: debug/platform/DebugPlatform/cmd 和 debug/platform/+/reply")

def on_message(c, u, msg):
    payload = msg.payload.decode("utf-8", errors="replace")
    log(f"[{time.strftime('%H:%M:%S')}] Python ← RECV topic={msg.topic} qos={msg.qos} len={len(msg.payload)} payload={payload!r}")

CLIENT_ID = "PartnerPy_" + str(int(time.time()))
# paho 2.x: CallbackAPIVersion.v2 是默认，但构造签名变了；
# 指定 CallbackAPIVersion.v1 以兼容旧签名
try:
    c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, client_id=CLIENT_ID, clean_session=True)
except AttributeError:
    # 旧版 1.x
    c = mqtt.Client(CLIENT_ID, clean_session=True)
c.on_connect = on_connect
c.on_message = on_message

log(f"[{time.strftime('%H:%M:%S')}] ============ MQTT 联调搭档启动 ============")
log(f"ClientId={CLIENT_ID}")
try:
    c.connect(BROKER, PORT, keepalive=60)
except Exception as e:
    log(f"[{time.strftime('%H:%M:%S')}] 连接 broker 失败: {e}")
    try:
        c.connect("test.mosquitto.org", 1883, keepalive=60)
        log(f"[{time.strftime('%H:%M:%S')}] 切换到备用 broker test.mosquitto.org 成功")
    except Exception as e2:
        log(f"[{time.strftime('%H:%M:%S')}] 备用 broker 也失败: {e2}")
        sys.exit(1)

c.loop_start()
time.sleep(3)

TOPIC = "debug/platform/devA/data"

# ① 单点 JSON
p1 = json.dumps({"addr": 0, "value": 12.34, "deviceId": "devA"})
c.publish(TOPIC, p1, qos=0)
log(f"[{time.strftime('%H:%M:%S')}] ① 发送单点 JSON → {TOPIC} = {p1}")
time.sleep(1.5)

# ② 数组 JSON
p2 = json.dumps({"data": [
    {"addr": 10, "value": 1.11},
    {"addr": 11, "value": 2.22},
    {"addr": 12, "value": 3.33}
]})
c.publish(TOPIC, p2, qos=0)
log(f"[{time.strftime('%H:%M:%S')}] ② 发送数组 JSON → {TOPIC} = {p2}")
time.sleep(1.5)

# ③ 纯数值（topic 末尾带地址 /data/5）
p3 = b"88.88"
TOPIC3 = "debug/platform/devA/data/5"
c.publish(TOPIC3, p3, qos=0)
log(f"[{time.strftime('%H:%M:%S')}] ③ 发送纯数值 → {TOPIC3} = {p3.decode()}")
time.sleep(1.5)

# ④ 高频连发 10 条 addr=20..29
for i in range(10):
    p = json.dumps({"addr": 20 + i, "value": 100.0 + i * 0.5})
    c.publish(TOPIC, p, qos=0)
log(f"[{time.strftime('%H:%M:%S')}] ④ 高频连发 10 条 → {TOPIC}  addr=20..29")
time.sleep(2)

# ⑤ 异常值
p5 = json.dumps({"addr": 99, "value": -9999.99, "deviceId": "devA", "error": "high_limit_exceeded"})
c.publish(TOPIC, p5, qos=0)
log(f"[{time.strftime('%H:%M:%S')}] ⑤ 发送异常值 → {TOPIC} = {p5}")
time.sleep(1.5)

log(f"[{time.strftime('%H:%M:%S')}] ============ 所有消息发送完毕，继续监听 15 秒 ============")
time.sleep(15)

c.loop_stop()
c.disconnect()
log(f"[{time.strftime('%H:%M:%S')}] 搭档退出")
