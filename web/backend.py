#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能头盔监控系统 - 后端服务器
订阅华为云MQTT数据并通过WebSocket推送到前端
"""

from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
from flask_cors import CORS
import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
from collections import deque
import threading

app = Flask(__name__)
app.config['SECRET_KEY'] = 'smart_helmet_secret_2026'
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# 数据存储（最近1000个数据点）
data_buffer = {
    'temp': deque(maxlen=1000),
    'humi': deque(maxlen=1000),
    'ppm': deque(maxlen=1000),
    'timestamps': deque(maxlen=1000)
}

# 最新数据
latest_data = {
    'temp': 0,
    'humi': 0,
    'ppm': 0,
    'dis_hr': 0,
    'dis_spo2': 0,
    'pitch': 0,
    'roll': 0,
    'yaw': 0,
    'fall_flag': 0,
    'collision_flag': 0,
    'heartrate_flag': 0,
    'spo2_flag': 0,
    'temp_flag': 0,
    'mq2_flag': 0,
    'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
}

# 华为云MQTT配置（使用应用侧账号订阅）
# 注意：设备侧账号只能发布，不能订阅。需要使用应用侧账号
MQTT_BROKER = "0c27d78187.st1.iotda-device.cn-north-4.myhuaweicloud.com"
MQTT_PORT = 1883
# 这里需要使用华为云应用侧的账号，而不是设备侧账号
# 如果没有应用侧账号，可以使用模拟数据模式
USE_SIMULATED_DATA = True  # 设置为True使用模拟数据

mqtt_client = None
mqtt_connected = False


def on_connect(client, userdata, flags, rc):
    """MQTT连接回调"""
    global mqtt_connected
    if rc == 0:
        print(f"[MQTT] 连接成功")
        mqtt_connected = True
        # 订阅设备上报的topic
        topic = "$oc/devices/698c76237f2e6c302f534208_Helmet_01/sys/properties/report"
        client.subscribe(topic)
        print(f"[MQTT] 订阅主题: {topic}")
    else:
        print(f"[MQTT] 连接失败，错误码: {rc}")
        mqtt_connected = False


def on_message(client, userdata, msg):
    """MQTT消息回调"""
    try:
        payload = json.loads(msg.payload.decode())
        print(f"[MQTT] 收到消息: {payload}")

        # 解析华为云物模型数据
        if 'services' in payload:
            for service in payload['services']:
                if 'properties' in service:
                    props = service['properties']
                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

                    # 更新最新数据
                    for key in props:
                        if key in latest_data:
                            latest_data[key] = props[key]
                    latest_data['timestamp'] = timestamp

                    # 更新历史数据（只保存温湿度和烟雾浓度）
                    if 'temp' in props:
                        data_buffer['temp'].append(props['temp'])
                        data_buffer['humi'].append(props.get('humi', 0))
                        data_buffer['ppm'].append(props.get('ppm', 0))
                        data_buffer['timestamps'].append(timestamp)

                    # 通过WebSocket推送到前端
                    socketio.emit('sensor_data', latest_data)

    except Exception as e:
        print(f"[MQTT] 解析消息失败: {e}")


def simulate_data():
    """模拟数据生成（用于测试）"""
    import random
    while True:
        if USE_SIMULATED_DATA:
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            # 生成模拟数据
            temp = 25 + random.uniform(-5, 10)
            humi = 60 + random.uniform(-20, 20)
            ppm = 50 + random.uniform(-30, 50)

            # 更新最新数据
            latest_data.update({
                'temp': round(temp, 1),
                'humi': round(humi, 1),
                'ppm': round(ppm, 2),
                'dis_hr': random.randint(60, 100),
                'dis_spo2': random.randint(95, 100),
                'pitch': random.uniform(-10, 10),
                'roll': random.uniform(-10, 10),
                'yaw': random.uniform(-180, 180),
                'timestamp': timestamp
            })

            # 更新历史数据
            data_buffer['temp'].append(latest_data['temp'])
            data_buffer['humi'].append(latest_data['humi'])
            data_buffer['ppm'].append(latest_data['ppm'])
            data_buffer['timestamps'].append(timestamp)

            # 推送到前端
            socketio.emit('sensor_data', latest_data)

        time.sleep(2)  # 每2秒更新一次


@app.route('/')
def index():
    """主页"""
    return render_template('index.html')


@app.route('/api/latest')
def get_latest():
    """获取最新数据"""
    return jsonify(latest_data)


@app.route('/api/history')
def get_history():
    """获取历史数据"""
    return jsonify({
        'temp': list(data_buffer['temp']),
        'humi': list(data_buffer['humi']),
        'ppm': list(data_buffer['ppm']),
        'timestamps': list(data_buffer['timestamps'])
    })


@socketio.on('connect')
def handle_connect():
    """WebSocket连接"""
    print('[WebSocket] 客户端连接')
    emit('sensor_data', latest_data)


@socketio.on('disconnect')
def handle_disconnect():
    """WebSocket断开"""
    print('[WebSocket] 客户端断开')


def start_mqtt():
    """启动MQTT客户端"""
    global mqtt_client

    if not USE_SIMULATED_DATA:
        mqtt_client = mqtt.Client()
        mqtt_client.on_connect = on_connect
        mqtt_client.on_message = on_message

        # 这里需要配置华为云应用侧的用户名和密码
        # mqtt_client.username_pw_set("your_app_username", "your_app_password")

        try:
            mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
            mqtt_client.loop_start()
            print("[MQTT] 客户端启动")
        except Exception as e:
            print(f"[MQTT] 连接失败: {e}")
            print("[提示] 切换到模拟数据模式")
    else:
        print("[模式] 使用模拟数据")
        # 启动模拟数据线程
        sim_thread = threading.Thread(target=simulate_data, daemon=True)
        sim_thread.start()


if __name__ == '__main__':
    print("=" * 50)
    print("智能头盔监控系统 - 后端服务器")
    print("=" * 50)
    print(f"模拟数据模式: {'开启' if USE_SIMULATED_DATA else '关闭'}")
    print("访问地址: http://localhost:5000")
    print("=" * 50)

    start_mqtt()
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)
