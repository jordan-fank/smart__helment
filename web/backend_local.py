#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能头盔监控系统 - 后端服务器（本地数据接收版）
接收ESP8266通过HTTP发送的数据并推送到前端
"""

from flask import Flask, render_template, jsonify, request
from flask_socketio import SocketIO, emit
from flask_cors import CORS
import json
import time
from datetime import datetime
from collections import deque
import threading

app = Flask(__name__)
app.config['SECRET_KEY'] = 'smart_helmet_secret_2026'
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# ========== 华为云 IoTDA 配置 ==========
HUAWEI_DEVICE_ID = "698c76237f2e6c302f534208_Helmet_01"

from huaweicloudsdkcore.auth.credentials import BasicCredentials
from huaweicloudsdkcore.http.http_config import HttpConfig
from huaweicloudsdkiotda.v5 import IoTDAClient, CreateCommandRequest, DeviceCommandRequest
from huaweicloudsdkcore.exceptions import exceptions as hw_exceptions

HUAWEI_AK = "HPUAAFRSZ5ZIVWHWHKCL"
HUAWEI_SK = "xS5BiO7Q98M2bPTV7xPnn8fJJyS5reh7rlgBOulB"
HUAWEI_PROJECT_ID = "1086fb47c9b44bb4a725ddaf5a1557c5"
HUAWEI_IOTDA_ENDPOINT = "https://0c27d78187.st1.iotda-app.cn-north-4.myhuaweicloud.com"

# 预初始化 IoTDA 客户端（启动时就创建，避免首次调用延迟）
iotda_client = None
iotda_client_lock = threading.Lock()

def get_iotda_client():
    """获取或创建 IoTDA 客户端（线程安全，短超时）"""
    global iotda_client
    if iotda_client is None:
        with iotda_client_lock:
            if iotda_client is None:
                credentials = BasicCredentials(HUAWEI_AK, HUAWEI_SK, HUAWEI_PROJECT_ID)
                credentials._derived_auth_service_name = 'iotda'
                credentials._region_id = 'cn-north-4'
                credentials = credentials.with_derived_predicate(BasicCredentials.get_default_derived_predicate())
                http_config = HttpConfig(timeout=(3, 5))
                iotda_client = (IoTDAClient.new_builder()
                                .with_credentials(credentials)
                                .with_endpoint(HUAWEI_IOTDA_ENDPOINT)
                                .with_http_config(http_config)
                                .build())
                print("[IoTDA] 客户端初始化成功（超时: 3s/5s）")
    return iotda_client

# 数据存储（最近100个数据点）
data_buffer = {
    'temp': deque(maxlen=100),
    'humi': deque(maxlen=100),
    'ppm': deque(maxlen=100),
    'dis_hr': deque(maxlen=100),
    'timestamps': deque(maxlen=100)
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
    'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
    'online': False
}

last_update_time = 0
data_count = 0


def check_device_status():
    """检查设备在线状态"""
    global last_update_time
    while True:
        current_time = time.time()
        # 如果超过10秒没有收到数据，标记为离线
        if current_time - last_update_time > 10:
            if latest_data['online']:
                latest_data['online'] = False
                socketio.emit('sensor_data', latest_data)
        time.sleep(5)


@app.route('/')
def index():
    """主页"""
    return render_template('index.html')


@app.route('/api/data', methods=['POST'])
def receive_data():
    """接收华为云转发的数据"""
    global last_update_time, data_count

    try:
        # 华为云转发的数据格式
        payload = request.get_json()
        print(f"[接收] 原始数据: {json.dumps(payload, ensure_ascii=False)}")

        # 解析华为云物模型数据 - 正确的路径是 notify_data.body.services
        services = None
        if payload:
            # 华为云数据转发格式
            if 'notify_data' in payload and 'body' in payload['notify_data']:
                services = payload['notify_data']['body'].get('services')
            # 直接格式（兼容）
            elif 'services' in payload:
                services = payload['services']

        if services:
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            for service in services:
                if 'properties' in service:
                    props = service['properties']

                    # 更新最新数据
                    for key in props:
                        if key in latest_data:
                            latest_data[key] = props[key]

                    latest_data['timestamp'] = timestamp
                    latest_data['online'] = True
                    last_update_time = time.time()
                    data_count += 1

                    # 更新历史数据
                    if 'temp' in props:
                        data_buffer['temp'].append(props['temp'])
                        data_buffer['timestamps'].append(timestamp)
                    if 'humi' in props:
                        data_buffer['humi'].append(props['humi'])
                    if 'ppm' in props:
                        data_buffer['ppm'].append(props['ppm'])
                    if 'dis_hr' in props:
                        data_buffer['dis_hr'].append(props['dis_hr'])

                    # 推送到前端
                    socketio.emit('sensor_data', latest_data)

                    print(f"[数据] #{data_count} 温度={props.get('temp', '--')} 心率={props.get('dis_hr', '--')} 在线=True")

            return jsonify({'status': 'ok', 'count': data_count}), 200
        else:
            print(f"[警告] 无法找到services数据")
            return jsonify({'status': 'error', 'message': '数据格式不正确'}), 400

    except Exception as e:
        print(f"[错误] 数据接收失败: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'status': 'error', 'message': str(e)}), 400


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
        'dis_hr': list(data_buffer['dis_hr']),
        'timestamps': list(data_buffer['timestamps'])
    })


@app.route('/api/status')
def get_status():
    """获取系统状态"""
    return jsonify({
        'online': latest_data['online'],
        'data_count': data_count,
        'last_update': latest_data['timestamp']
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


def _send_to_cloud(device, action):
    """异步下发命令到华为云（在线程中调用）"""
    try:
        client = get_iotda_client()
        cmd_body = DeviceCommandRequest(
            service_id="Control",
            command_name="Switch_Control",
            paras={"device": device, "action": action}
        )
        req = CreateCommandRequest(device_id=HUAWEI_DEVICE_ID, body=cmd_body)
        client.create_command(req)
        print(f"[命令] {device} -> {action}, 华为云下发成功")
    except hw_exceptions.ClientRequestException as e:
        if 'IOTDA.014111' in str(e.error_msg):
            print(f"[命令] {device} -> {action}, 已下发（设备未响应确认）")
        else:
            print(f"[命令] 华为云错误: {e.error_msg}")
    except Exception as e:
        print(f"[命令] 下发失败: {e}")


@app.route('/api/command', methods=['POST'])
def send_command():
    """下发设备控制命令到华为云IoTDA（异步，不阻塞响应）"""
    try:
        payload = request.get_json(force=True)
        if not payload:
            return jsonify({'status': 'error', 'message': '请求体为空'}), 400

        device = payload.get('device', '').upper()
        action = payload.get('action', '').upper()
        print(f"[命令] 收到请求: device={device}, action={action}")

        if device not in ('LED', 'FAN') or action not in ('ON', 'OFF'):
            return jsonify({'status': 'error', 'message': '参数无效'}), 400

        threading.Thread(target=_send_to_cloud, args=(device, action), daemon=True).start()
        return jsonify({'status': 'ok', 'device': device, 'action': action}), 200

    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({'status': 'error', 'message': str(e)}), 500


if __name__ == '__main__':
    print("=" * 60)
    print("智能头盔监控系统 - 后端服务器")
    print("=" * 60)
    print("数据源: 华为云IoT数据转发")
    print("访问地址: http://localhost:5000")
    print("数据接收: POST http://localhost:5000/api/data")
    print()

    # 预初始化华为云客户端（避免首次命令延迟）
    def warmup_iotda():
        try:
            get_iotda_client()
            print("[IoTDA] 客户端预热完成")
        except Exception as e:
            print(f"[IoTDA] 预热失败（首次命令时重试）: {e}")
    threading.Thread(target=warmup_iotda, daemon=True).start()

    # 启动设备状态检查线程
    status_thread = threading.Thread(target=check_device_status, daemon=True)
    status_thread.start()

    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)
