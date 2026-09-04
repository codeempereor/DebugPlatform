# -*- coding: utf-8 -*-
"""MQTT 联调搭档（v2，发送到 devices/<deviceId>/data 匹配当前 GUI 默认订阅）"""
import time, sys, json, random
try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("需要: pip install paho-mqtt"); sys.exit(1)

BROKER = "broker.emqx.io"
PORT   = 1883
DEV_ID = "devA"
PUB_TOPIC  = f"devices/{DEV_ID}/data"
PUB_TOPIC5 = f"devices/{DEV_ID}/data/5"   # 纯数值，地址从 topic 尾段取
REPLY_SUB  = f"devices/{DEV_ID}/reply"
CMD_SUB    = f"debug/platform/DebugPlatform/cmd"
CLIENT_ID  = f"PartnerPy2_{int(time.time())}"

def on_connect(c, u, f, rc):
    print(f"[{{t}}] Python 连接 broker {{broker}}:{{port}}  rc={{rc}}".format(
        t=time.strftime("%H:%M:%S"), broker=BROKER, port=PORT, rc=rc))
    c.subscribe(CMD_SUB); c.subscribe(REPLY_SUB)
    print(f"  订阅: {{CMD}} 和 {{REPLY}}".format(CMD=CMD_SUB, REPLY=REPLY_SUB))

def on_msg(c, u, m):
    try:
        payload = m.payload.decode("utf-8", errors="replace")
    except Exception:
        payload = str(m.payload)
    print(f"  [{time.strftime('%H:%M:%S')}] RX {m.topic} = {payload[:200]}")

def ts(): return time.strftime("%H:%M:%S")

c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, client_id=CLIENT_ID, clean_session=True)
c.on_connect = on_connect; c.on_message = on_msg
print(f"[{ts()}] ============ MQTT 联调搭档 v2 启动 ============\nClientId={CLIENT_ID}")
c.connect(BROKER, PORT, keepalive=60); c.loop_start()
time.sleep(2.0)

steps = [
    ("① 单点 JSON",    PUB_TOPIC,  {"addr":0,"value":12.34,"deviceId":"devA"}),
    ("② 数组 JSON",    PUB_TOPIC,  {"data":[{"addr":10,"value":1.11},{"addr":11,"value":2.22},{"addr":12,"value":3.33}]}),
    ("③ 纯数值(路径)", PUB_TOPIC5, 88.88),
]
for name, topic, payload in steps:
    p = payload if isinstance(payload, (str, bytes)) else json.dumps(payload, ensure_ascii=False)
    print(f"[{ts()}] {name} -> {topic} = {p}")
    c.publish(topic, p, qos=0)
    time.sleep(1.0)

print(f"[{ts()}] ④ 高频连发 10 条 -> {PUB_TOPIC} addr=20..29")
for i in range(10):
    a = 20 + i
    p = json.dumps({"addr":a,"value":round(random.uniform(0,100),2),"deviceId":"devA"}, ensure_ascii=False)
    c.publish(PUB_TOPIC, p, qos=0)
    time.sleep(0.05)
time.sleep(1.0)

print(f"[{ts()}] ⑤ 异常值 -> {PUB_TOPIC} addr=99 error=high_limit_exceeded")
c.publish(PUB_TOPIC, json.dumps({"addr":99,"value":-9999.99,"deviceId":"devA","error":"high_limit_exceeded"}, ensure_ascii=False), qos=0)

print(f"[{ts()}] ============ 所有消息发送完毕，继续监听 15 秒 ============")
time.sleep(15)
c.loop_stop(); c.disconnect()
print(f"[{ts()}] 搭档退出")
