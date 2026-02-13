// 后端API配置
const API_CONFIG = {
    baseUrl: 'http://localhost:5000',  // 后端服务器地址
    endpoints: {
        latest: '/api/latest',
        history: '/api/history',
        status: '/api/status'
    }
};

// 数据存储
let sensorData = {
    temp: [],
    humi: [],
    ppm: [],
    dis_hr: [],
    dis_spo2: [],
    pitch: [],
    roll: [],
    yaw: [],
    latitude: 0,
    longitude: 0,
    timestamps: []
};

let packetCount = 0;
let map = null;
let marker = null;

// 初始化图表
let tempChart, hrChart;

function initCharts() {
    // 温度图表
    const tempCtx = document.getElementById('tempChart').getContext('2d');
    tempChart = new Chart(tempCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: '温度 (°C)',
                data: [],
                borderColor: 'rgb(255, 99, 132)',
                backgroundColor: 'rgba(255, 99, 132, 0.1)',
                tension: 0.4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: false
                }
            },
            plugins: {
                legend: {
                    display: true
                }
            }
        }
    });

    // 心率图表
    const hrCtx = document.getElementById('hrChart').getContext('2d');
    hrChart = new Chart(hrCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: '心率 (bpm)',
                data: [],
                borderColor: 'rgb(75, 192, 192)',
                backgroundColor: 'rgba(75, 192, 192, 0.1)',
                tension: 0.4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: false,
                    min: 40,
                    max: 140
                }
            },
            plugins: {
                legend: {
                    display: true
                }
            }
        }
    });
}

// 初始化地图
function initMap() {
    map = new AMap.Map('map', {
        zoom: 15,
        center: [116.397428, 39.90923] // 默认北京
    });

    marker = new AMap.Marker({
        position: [116.397428, 39.90923],
        title: '智能头盔位置'
    });
    map.add(marker);
}

// 更新UI
function updateUI(data) {
    // 更新环境数据
    if (data.temp !== undefined) {
        document.getElementById('temp').textContent = data.temp.toFixed(1) + ' °C';
        updateChart(tempChart, data.temp, sensorData.temp);
    }
    if (data.humi !== undefined) {
        document.getElementById('humi').textContent = data.humi.toFixed(1) + ' %';
    }
    if (data.ppm !== undefined) {
        document.getElementById('ppm').textContent = data.ppm.toFixed(2) + ' ppm';
    }

    // 更新生理数据
    if (data.dis_hr !== undefined) {
        document.getElementById('dis_hr').textContent = data.dis_hr + ' bpm';
        updateChart(hrChart, data.dis_hr, sensorData.dis_hr);
    }
    if (data.dis_spo2 !== undefined) {
        document.getElementById('dis_spo2').textContent = data.dis_spo2 + ' %';
    }

    // 更新姿态数据
    if (data.Pitch !== undefined) {
        document.getElementById('pitch').textContent = data.Pitch.toFixed(1) + ' °';
    }
    if (data.Roll !== undefined) {
        document.getElementById('roll').textContent = data.Roll.toFixed(1) + ' °';
    }
    if (data.Yaw !== undefined) {
        document.getElementById('yaw').textContent = data.Yaw.toFixed(1) + ' °';
    }

    // 更新GPS
    if (data.latitude !== undefined && data.longitude !== undefined) {
        document.getElementById('latitude').textContent = data.latitude.toFixed(6);
        document.getElementById('longitude').textContent = data.longitude.toFixed(6);

        // 更新地图标记
        if (map && marker && data.latitude !== 0 && data.longitude !== 0) {
            const position = [data.longitude, data.latitude];
            marker.setPosition(position);
            map.setCenter(position);
        }
    }

    // 更新安全状态
    updateSafetyStatus('fall_flag', data.fall_flag, '跌倒');
    updateSafetyStatus('collision_flag', data.collision_flag, '碰撞');
    updateSafetyStatus('temp_flag', data.temp_flag, '温度过高');
    updateSafetyStatus('mq2_flag', data.mq2_flag, '烟雾');

    // 更新状态栏
    document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
    document.getElementById('packetCount').textContent = ++packetCount;
}

// 更新安全状态
function updateSafetyStatus(id, value, alertName) {
    const element = document.getElementById(id);
    if (value === 1 || value === true) {
        element.textContent = '⚠️ 报警';
        element.style.color = '#ef4444';
        showAlert(`${alertName}报警！`);
    } else {
        element.textContent = '✅ 正常';
        element.style.color = '#10b981';
    }
}

// 显示报警
function showAlert(message) {
    const alertBox = document.getElementById('alertBox');
    const alertMessage = document.getElementById('alertMessage');
    alertMessage.textContent = message;
    alertBox.classList.add('show');

    // 5秒后自动隐藏
    setTimeout(() => {
        alertBox.classList.remove('show');
    }, 5000);
}

// 更新图表
function updateChart(chart, newValue, dataArray) {
    const maxDataPoints = 20;

    // 添加新数据
    dataArray.push(newValue);
    if (dataArray.length > maxDataPoints) {
        dataArray.shift();
    }

    // 更新图表
    chart.data.labels = dataArray.map((_, i) => i);
    chart.data.datasets[0].data = dataArray;
    chart.update('none'); // 不使用动画，提高性能
}

// 模拟数据（用于测试）
function generateMockData() {
    return {
        temp: 25 + Math.random() * 10,
        humi: 50 + Math.random() * 20,
        ppm: Math.random() * 100,
        dis_hr: 70 + Math.random() * 30,
        dis_spo2: 95 + Math.random() * 5,
        Pitch: Math.random() * 20 - 10,
        Roll: Math.random() * 20 - 10,
        Yaw: Math.random() * 360,
        latitude: 39.90923 + (Math.random() - 0.5) * 0.01,
        longitude: 116.397428 + (Math.random() - 0.5) * 0.01,
        fall_flag: Math.random() > 0.9 ? 1 : 0,
        collision_flag: Math.random() > 0.95 ? 1 : 0,
        temp_flag: Math.random() > 0.9 ? 1 : 0,
        mq2_flag: Math.random() > 0.9 ? 1 : 0
    };
}

// 从后端API获取数据
async function fetchDataFromBackend() {
    try {
        const response = await fetch(`${API_CONFIG.baseUrl}${API_CONFIG.endpoints.latest}`);

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();

        // 检查设备是否在线
        if (data.online) {
            document.getElementById('deviceStatus').textContent = '在线';
            document.getElementById('deviceStatus').classList.add('status-online');
            document.getElementById('deviceStatus').classList.remove('status-offline');
            updateUI(data);
        } else {
            document.getElementById('deviceStatus').textContent = '离线';
            document.getElementById('deviceStatus').classList.remove('status-online');
            document.getElementById('deviceStatus').classList.add('status-offline');
        }

    } catch (error) {
        console.error('获取数据失败:', error);
        document.getElementById('deviceStatus').textContent = '连接失败';
        document.getElementById('deviceStatus').classList.remove('status-online');
        document.getElementById('deviceStatus').classList.add('status-offline');
    }
}

// 初始化
document.addEventListener('DOMContentLoaded', () => {
    initCharts();

    // 检查是否有高德地图API key
    if (typeof AMap !== 'undefined') {
        initMap();
    } else {
        document.getElementById('map').innerHTML = '<div class="loading">请配置高德地图API Key</div>';
    }

    // 定时获取数据（每2秒）
    setInterval(fetchDataFromBackend, 2000);

    // 立即获取一次
    fetchDataFromBackend();
});

// WebSocket连接（可选方案，需要后端支持）
function connectWebSocket() {
    const ws = new WebSocket('ws://your-backend-server:8080/ws');

    ws.onopen = () => {
        console.log('WebSocket连接成功');
        document.getElementById('deviceStatus').textContent = '在线';
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        updateUI(data);
    };

    ws.onerror = (error) => {
        console.error('WebSocket错误:', error);
    };

    ws.onclose = () => {
        console.log('WebSocket连接关闭，5秒后重连...');
        setTimeout(connectWebSocket, 5000);
    };
}

// 如果需要使用WebSocket，取消下面的注释
// connectWebSocket();