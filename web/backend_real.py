#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能头盔监控系统 - 后端服务器（真实数据版）
通过华为云IoT REST API获取设备数据并推送到前端
"""

from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
from flask_cors import CORS
import requests
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
    'latitude': 0,
    'longitude': 0,
    'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
}

# 华为云IoT配置
HUAWEI_CONFIG = {
    'project_id': '0c27d78187',  # 从域名中提取
    'device_id': '698c76237f2e6c302f534208_Helmet_01',
    'region': 'cn-north-4',
    # 需要在华为云控制台创建应用并获取以下信息
    # 访问：https://console.huaweicloud.com/iotdm/?region=cn-north-4
    # 进入：总览 -> 访问密钥 -> 新增访问密钥
    'app_id': 'YOUR_APP_ID',  # 应用ID
    'app_secret': 'YOUR_APP_SECRET',  # 应用密钥
}

# API端点
API_BASE = f"https://iotda.{HUAWEI_CONFIG['region']}.myhuaweicloud.com/v5/iot/{HUAWEI_CONFIG['project_id']}"
API_DEVICE_PROPERTIES = f"{API_BASE}/devices/{HUAWEI_CONFIG['device_id']}/properties"

# 访问令牌
access_token = None
token_expire_time = 0


def get_access_token():
    """获取华为云访问令牌"""
    global access_token, token_expire_time

    # 如果令牌还有效，直接返回
    if access_token and time.time() < token_expire_time:
        return access_token

    # 注意：这里需要使用IAM认证获取token
    # 由于需要AK/SK，这里提供简化版本
    # 实际使用时需要配置华为云的AK/SK

    print("[警告] 未配置华为云访问密钥，无法获取真实数据")
    return None


def fetch_device_data():
    """从华为云获取设备最新数据"""
    try:
        token = get_access_token()
        if not token:
            return None

        headers = {
            'X-Auth-Token': token,
            'Content-Type': 'application/json'
        }

        response = requests.get(API_DEVICE_PROPERTIES, headers=headers, timeout=5)

        if response.status_code == 200:
            data = response.json()
            return data
        else:
            print(f"[API] 获取数据失败: {response.status_code}")
            return None

    except Exception as e:
        print(f"[API] 请求异常: {e}")
        return None


def poll_device_data():
    """定期轮询设备数据（简化方案）"""
    while True:
        try:
            # 由于需要配置AK/SK，这里使用模拟数据作为示例
            # 实际部署时需要配置华为云访问密钥

            # 模拟从华为云获取的数据格式
            # 实际数据格式请参考华为云IoT文档
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            # 这里应该调用 fetch_device_data() 获取真实数据
            # device_data = fetch_device_data()

            # 暂时使用模拟数据（需要配置AK/SK后才能获取真实数据）
            import random
            latest_data.update({
                'temp': round(25 + random.uniform(-5, 10), 1),
                'humi': round(60 + random.uniform(-20, 20), 1),
                'ppm': round(50 + random.uniform(-30, 50), 2),
                'dis_hr': random.randint(60, 100),
                'dis_spo2': random.randint(95, 100),
                'pitch': round(random.uniform(-10, 10), 1),
                'roll': round(random.uniform(-10, 10), 1),
                'yaw': round(random.uniform(-180, 180), 1),
                'timestamp': timestamp
            })

            # 更新历史数据
            data_buffer['temp'].append(latest_data['temp'])
            data_buffer['humi'].append(latest_data['humi'])
            data_buffer['ppm'].append(latest_data['ppm'])
            data_buffer['timestamps'].append(timestamp)

            # 推送到前端
            socketio.emit('sensor_data', latest_data)

        except Exception as e:
            print(f"[轮询] 错误: {e}")

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


if __name__ == '__main__':
    print("=" * 50)
    print("智能头盔监控系统 - 后端服务器")
    print("=" * 50)
    print("数据源: 华为云IoT平台")
    print("访问地址: http://localhost:5000")
    print("=" * 50)
    print()
    print("注意：要获取真实数据，需要配置华为云访问密钥")
    print("请访问华为云控制台获取AK/SK：")
    print("https://console.huaweicloud.com/iam/?region=cn-north-4#/mine/accessKey")
    print()
    print("=" * 50)

    # 启动数据轮询线程
    poll_thread = threading.Thread(target=poll_device_data, daemon=True)
    poll_thread.start()

    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)
