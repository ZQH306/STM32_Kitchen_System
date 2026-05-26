# server.py
from fastmcp import FastMCP
import sys
import logging


import time
import threading
import paho.mqtt.client as mqtt

# 全局MQTT客户端和线程锁
mqtt_client = None
mqtt_lock = threading.Lock()

sensor_data = {}  # 存储所有传感器数据
device_states = {}  # 存储所有设备状态


def on_mqtt_connect(client, userdata, flags, reason_code, properties):
    """MQTT连接回调 - VERSION2 签名"""
    # reason_code 对应原来的 rc
    if reason_code == 0:
        logger.info("✅ 成功连接到MQTT服务器")

        # 连接成功后订阅传感器和设备状态主题
        try:
            # 订阅传感器主题（通配符+）
            client.subscribe("huahua/sensor/+", qos=1)
            logger.info("📡 已订阅传感器主题: huahua/sensor/+")

            # 订阅设备状态主题（通配符+）
            client.subscribe("huahua/state/+", qos=1)
            logger.info("📡 已订阅设备状态主题: huahua/state/+")         

        except Exception as e:
            logger.error(f"❌ 订阅主题失败: {e}")

    else:
        # VERSION2 中 reason_code 是 mqtt.ReasonCode 对象
        if hasattr(reason_code, 'value'):
            reason_value = reason_code.value
        else:
            reason_value = reason_code

        error_messages = {
            1: "不正确的协议版本",
            2: "无效的客户端标识符",
            3: "服务器不可用",
            4: "错误的用户名或密码",
            5: "未授权"
        }
        error_msg = error_messages.get(reason_value, f"未知错误码: {reason_value}")
        logger.error(f"❌ MQTT连接失败: {error_msg}")

# 修改 on_mqtt_disconnect 函数，使其适应 VERSION2 的回调API
def on_mqtt_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    """MQTT断开连接回调 - VERSION2 签名"""
    if reason_code == 0:
        logger.info("MQTT连接正常关闭")
    else:
        # VERSION2 中 reason_code 是 mqtt.ReasonCode 对象
        if hasattr(reason_code, 'value'):
            reason_value = reason_code.value
        else:
            reason_value = reason_code

        logger.warning(f"⚠️ MQTT连接意外断开，错误码: {reason_value}")


def on_mqtt_message(client, userdata, message):
    """MQTT消息接收回调 - 处理传感器和设备状态数据"""
    global sensor_data, device_states
    
    topic = message.topic
    payload = message.payload.decode('utf-8')

    if message.retain:
        logger.debug(f"📨 收到保留消息 - 主题: {topic}, 内容: {payload}")
        return  # 忽略保留消息
    
    logger.debug(f"📨 收到消息 - 主题: {topic}, 内容: {payload}")
    
    try:      
        data = payload

        # 根据主题类型处理数据
        if topic.startswith("huahua/sensor/"):
            # 传感器数据处理
            sensor_id = topic.split("/")[-1]  # 获取传感器ID
            
            # 更新传感器数据存储
            sensor_data[sensor_id] = {
                "value": data if isinstance(data, (int, float, str)) else data,
                "timestamp": time.time()
            }
            
            logger.info(f"📊 传感器数据更新 - {sensor_id}: {data}")
                
        elif topic.startswith("huahua/state/"):
            # 设备状态处理
            device_id = topic.split("/")[-1]  # 获取设备ID
            
            # 更新设备状态存储
            device_states[device_id] = {
                "state": data if isinstance(data, (int, float, str, bool)) else data,
                "timestamp": time.time()
            }
            
            logger.info(f"🔌 设备状态更新 - {device_id}: {data}")
                            
    except Exception as e:
        logger.error(f"❌ 处理MQTT消息时出错: {e}, 主题: {topic}, 负载: {payload}")


def connect_mqtt():
    """连接MQTT服务器（使用库内置的自动重连），失败则退出程序"""
    global mqtt_client
    
    try:
        logger.info("🔗 正在连接MQTT服务器 (localhost:1883)...")
        
        # 创建MQTT客户端
        mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        
        # 设置回调函数
        mqtt_client.on_connect = on_mqtt_connect
        mqtt_client.on_disconnect = on_mqtt_disconnect
        mqtt_client.on_message = on_mqtt_message
        
        # 启用库内置的自动重连功能
        # 最小重连延迟1秒，最大30秒
        mqtt_client.reconnect_delay_set(min_delay=1, max_delay=30)
        
        # 连接到MQTT服务器
        mqtt_client.connect("itmojun.com", 1883, 60)
        
        # 启动网络循环（后台线程）
        mqtt_client.loop_start()
        
        # 等待连接建立，最长等待10秒
        logger.info("等待MQTT连接建立...")
        for i in range(20):  # 20 * 0.5 = 10秒
            time.sleep(0.5)
            if mqtt_client.is_connected():
                logger.info("✅ MQTT连接已建立并准备就绪")
                return True
        
        # 连接超时
        logger.error("❌ MQTT连接超时")
        return False
        
    except Exception as e:
        logger.error(f"❌ 连接MQTT服务器失败: {e}")
        return False

def publish_mqtt_message(topic: str, message: str, qos: int = 1) -> bool:
    """发布MQTT消息（线程安全）"""
    global mqtt_client
   
    with mqtt_lock:
        try:
            # 检查连接状态
            if not mqtt_client.is_connected():
                logger.warning("MQTT连接已断开，等待自动重连...")
                # 库会自动重连，这里我们等待一下
                # 在实际使用中，可以增加重试逻辑或返回失败
                return False
            
            # 发布消息
            result = mqtt_client.publish(topic, message, qos=qos)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"📤 已发布消息到主题 {topic}: {message}")
                return True
            else:
                logger.error(f"发布消息失败，错误码: {result.rc}")
                return False
                
        except Exception as e:
            logger.error(f"发布MQTT消息时出错: {e}")
            return False

def cleanup_mqtt():
    """清理MQTT连接"""
    global mqtt_client
    
    if mqtt_client:
        try:
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
            logger.info("✅ MQTT连接已关闭")
        except Exception as e:
            logger.error(f"关闭MQTT连接时出错: {e}")
        finally:
            mqtt_client = None


logger = logging.getLogger('Calculator')

logger.setLevel(logging.INFO)  # 设置本级日志级别

# 创建并配置控制台处理器
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)

# 将处理器添加到logger
logger.addHandler(ch)

# Fix UTF-8 encoding for Windows console
if sys.platform == 'win32':
    sys.stderr.reconfigure(encoding='utf-8')
    sys.stdout.reconfigure(encoding='utf-8')

import math
import random

# Create an MCP server
mcp = FastMCP("huahua")

# Add an addition tool
# @mcp.tool()
# def calculator(python_expression: str) -> dict:
#     """For mathamatical calculation, always use this tool to calculate the result of a python expression. You can use 'math' or 'random' directly, without 'import'."""
#     result = eval(python_expression, {"math": math, "random": random})
#     logger.info(f"Calculating formula: {python_expression}, result: {result}")
#     return {"success": True, "result": result}


# 添加打开室内LED灯工具
@mcp.tool()
def lamp_turn_on() -> dict:
    """
    打开室内LED灯，因为室内LED灯随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出开灯指令时先直接调用 “lamp_get_state” 工具获取LED灯当前状态，如果LED灯已经是打开状态则不需要调用此工具；
    用户控制指令可能是“开灯”、“打开LED灯”、“点亮LED灯”、“打开室内灯”、“打开室内LED灯” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"lamp_turn_on called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "a")

    if success:
        logger.info(f"打开室内LED灯成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"打开室内LED灯失败！")
        return {"success": False, "result": "控制失败"}


# 添加关闭室内LED灯工具
@mcp.tool()
def lamp_turn_off() -> dict:
    """
    关闭室内LED灯，因为室内LED灯随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出关灯指令时先直接调用 “lamp_get_state” 工具获取LED灯当前状态，如果LED灯已经是关闭状态则不需要调用此工具；
    用户控制指令可能是“关灯”、“关闭LED灯”、“熄灭LED灯”、“关闭室内灯”、“关闭室内LED灯” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"lamp_turn_off called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "b")

    if success:
        logger.info(f"关闭室内LED灯成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"关闭室内LED灯失败！")
        return {"success": False, "result": "控制失败"}


# 添加获取室内LED灯状态工具
@mcp.tool()
def lamp_get_state() -> dict:
    """
    获取室内LED灯状态，1 表示开，0 表示关，未知表示状态未知，设备不在线或异常。返回结果示例：{"success": True, "state": 1}
    无论何情况下都必须调用此工具获取当前LED灯状态，不能假设LED灯状态与上次控制时相同。 
    用户控制指令可能是“查看灯的状态”、“看下灯开了没”、“室内灯关了吗”、“看下灯的状态”、“灯开了吗” 等  
    """
    logger.info(f"lamp_get_state called")
    global device_states
    state_info = device_states.get("led", None)   
    if state_info:
        state = state_info["state"]
        logger.info(f"室内LED灯状态获取成功，状态: {state}")
        return {"success": True, "state": state}
    else:
        logger.warning(f"室内LED灯状态未知")
        return {"success": False, "state": "未知"}




# 添加打开室内风扇工具
@mcp.tool()
def fan_turn_on() -> dict:
    """
    打开室内风扇，因为室内风扇随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出打开风扇指令时先直接调用 “fan_get_state” 工具获取风扇当前状态，如果风扇已经是打开状态则不需要调用此工具；
    用户控制指令可能是“开风扇”、“打开风扇”、“启动风扇”、“开室内风扇”、“打开室内风扇” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"fan_turn_on called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "e")

    if success:
        logger.info(f"打开室内风扇1档成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"打开室内风扇1档失败！")
        return {"success": False, "result": "控制失败"}


# 添加风扇2档工具
@mcp.tool()
def fan_level_2() -> dict:
    """
    打开室内风扇2档，因为室内风扇随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出开2档指令时先直接调用 “fan_get_state” 工具获取风扇当前状态；
    用户控制指令可能是“风扇2档”、“开风扇2档”、“调风扇到2档” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"fan_level_2 called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "f")

    if success:
        logger.info(f"打开室内风扇2档成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"打开室内风扇2档失败！")
        return {"success": False, "result": "控制失败"}


# 添加风扇3档工具
@mcp.tool()
def fan_level_3() -> dict:
    """
    打开室内风扇3档，因为室内风扇随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出开3档指令时先直接调用 “fan_get_state” 工具获取风扇当前状态；
    用户控制指令可能是“风扇3档”、“开风扇3档”、“调风扇到3档”、“开最大风” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"fan_level_3 called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "g")

    if success:
        logger.info(f"打开室内风扇3档成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"打开室内风扇3档失败！")
        return {"success": False, "result": "控制失败"}


# 添加关闭室内风扇工具
@mcp.tool()
def fan_turn_off() -> dict:
    """
    关闭室内风扇，因为室内风扇随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出关风扇指令时先直接调用 “fan_get_state” 工具获取风扇当前状态，如果风扇已经是关闭状态则不需要调用此工具；
    用户控制指令可能是“关风扇”、“关闭风扇”、“停止风扇”、“关闭室内风扇”、“关室内风扇” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"fan_turn_off called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "h")

    if success:
        logger.info(f"关闭室内风扇成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"关闭室内风扇失败！")
        return {"success": False, "result": "控制失败"}


# 添加获取室内风扇状态工具
@mcp.tool()
def fan_get_state() -> dict:
    """
    获取室内风扇状态，0 表示关闭，1 表示1档，2 表示2档，3 表示3档，未知表示状态未知，设备不在线或异常。返回结果示例：{"success": True, "state": 1}
    无论何情况下都必须调用此工具获取当前风扇状态，不能假设风扇状态与上次控制时相同。 
    用户控制指令可能是“查看风扇的状态”、“看下风扇开了没”、“室内风扇关了吗”、“看下风扇的状态”、“风扇开了吗” 等  
    """
    logger.info(f"fan_get_state called")
    global device_states
    state_info = device_states.get("fan", None)   
    if state_info:
        state = state_info["state"]
        logger.info(f"室内风扇状态获取成功，状态: {state}")
        return {"success": True, "state": state}
    else:
        logger.warning(f"室内风扇状态未知")
        return {"success": False, "state": "未知"}




# 添加打开室内蜂鸣器/报警器工具
@mcp.tool()
def buzzer_turn_on() -> dict:
    """
    打开室内蜂鸣器/报警器，因为室内蜂鸣器/报警器随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出打开蜂鸣器指令时先直接调用 “buzzer_get_state” 工具获取蜂鸣器当前状态，如果蜂鸣器已经是打开状态则不需要调用此工具；
    用户控制指令可能是“开蜂鸣器”、“打开蜂鸣器”、“启动蜂鸣器”、“打开室内蜂鸣器”、“启动室内蜂鸣器”、“打开报警器”、“启动报警器” 、“打开室内报警器”、“开始报警”、“报警”、“发出报警声”等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"buzzer_turn_on called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "c")

    if success:
        logger.info(f"打开室内蜂鸣器成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"打开室内蜂鸣器失败！")
        return {"success": False, "result": "控制失败"}


# 添加关闭室内蜂鸣器/报警器工具
@mcp.tool()
def buzzer_turn_off() -> dict:
    """
    关闭室内蜂鸣器/报警器，因为室内蜂鸣器/报警器随时可能被物理开关或其他方式控制而改变状态，所以每次用户发出关闭蜂鸣器指令时先直接调用 “buzzer_get_state” 工具获取蜂鸣器当前状态，如果蜂鸣器已经是关闭状态则不需要调用此工具；
    用户控制指令可能是“关蜂鸣器”、“关闭蜂鸣器”、“停止蜂鸣器”、“关闭室内蜂鸣器”、“停止室内蜂鸣器”、“关闭报警器”、“停止报警器”、“关闭室内报警器”、“静音”、“停止报警” 等，该工具返回结果示例：{"success": True, "result": "控制成功"}；
    """
    logger.info(f"buzzer_turn_off called")

    # 发布MQTT消息
    success = publish_mqtt_message("huahua/cmd", "d")

    if success:
        logger.info(f"关闭室内蜂鸣器成功！")
        return {"success": True, "result": "控制成功"}
    else:
        # 如果发布失败
        logger.error(f"关闭室内蜂鸣器失败！")
        return {"success": False, "result": "控制失败"}


# 添加获取室内蜂鸣器/报警器状态工具
@mcp.tool()
def buzzer_get_state() -> dict:
    """
    获取室内蜂鸣器/报警器状态，1 表示开，0 表示关，未知表示状态未知，设备不在线或异常。返回结果示例：{"success": True, "state": 1}
    无论何情况下都必须调用此工具获取当前蜂鸣器/报警器状态，不能假设蜂鸣器/报警器状态与上次控制时相同。 
    用户控制指令可能是“查看蜂鸣器的状态”、“听下蜂鸣器在叫吗”、“室内蜂鸣器关了吗”、“看下蜂鸣器的状态”、“蜂鸣器开了吗” 、“蜂鸣器在叫吗”、“报警器开了吗”、“查一下报警器状态”、“室内报警器在报警吗”、“室内报警器响了吗”等  
    """
    logger.info(f"buzzer_get_state called")
    global device_states
    state_info = device_states.get("buzzer", None)   
    if state_info:
        state = state_info["state"]
        logger.info(f"室内蜂鸣器状态获取成功，状态: {state}")
        return {"success": True, "state": state}
    else:
        logger.warning(f"室内蜂鸣器状态未知")
        return {"success": False, "state": "未知"}



# 添加获取室内温度工具
@mcp.tool()
def get_temperature() -> dict:
    """
    获取室内温度，返回结果示例：{"success": True, "temperature": 25.5℃}
    无论何情况下都必须调用此工具获取当前实时温度，不能假设温度与上次相同。 
    用户控制指令可能是“查看温度”、“看下室内温度”、“室内现在多少度”、“看下温度”、“温度多少了” 等。
    当用户问室内是冷还是热时，请先调用此工具获取温度，然后根据温度值判断：低于20度为冷，20-26度为适中，高于26度为热，并将判断结果反馈给用户。 
    """
    logger.info(f"get_temperature called")
    global sensor_data
    data = sensor_data.get("dht11", None)   
    if data:
        temperature = data["value"].split("_")[0]  # 格式为 "温度_湿度"
        logger.info(f"室内温度获取成功，温度: {temperature}℃")
        return {"success": True, "temperature": temperature + "℃"}
    else:
        logger.warning(f"室内温度未知")
        return {"success": False, "temperature": "未知，温度传感器异常或离线"}


# 添加获取室内湿度工具
@mcp.tool()
def get_humidity() -> dict:
    """
    获取室内湿度，返回结果示例：{"success": True, "humidity": 60.5%}
    无论何情况下都必须调用此工具获取当前实时湿度，不能假设湿度与上次相同。 
    用户控制指令可能是“查看湿度”、“看下室内湿度”、“室内现在多少湿度”、“看下湿度”、“湿度多少了” 等。
    当用户问室内是干燥还是潮湿时，不要随意口头回答，请先调用此工具获取湿度，然后根据湿度值判断：低于30%为干燥，30%-60%为适中，高于60%为潮湿，并将判断结果反馈给用户。
    """
    logger.info(f"get_humidity called")
    global sensor_data
    data = sensor_data.get("dht11", None)   
    if data:
        humidity = data["value"].split("_")[1]  # 格式为 "温度_湿度"
        logger.info(f"室内湿度获取成功，湿度: {humidity}%")
        return {"success": True, "humidity": humidity + "%"}
    else:
        logger.warning(f"室内湿度未知")
        return {"success": False, "humidity": "未知，温湿度传感器异常或离线"}


# 添加获取室内光照强度工具
@mcp.tool()
def get_light() -> dict:
    """
    获取室内光照强度，返回结果示例：{"success": True, "light": 200}
    无论何情况下都必须调用此工具获取当前实时光照强度，不能假设光照与上次相同。 
    用户控制指令可能是“查看光照”、“看下现在光照”、“现在多少光”、“查看亮度”、“亮不亮” 等。
    """
    logger.info(f"get_light called")
    global sensor_data
    data = sensor_data.get("light", None)   
    if data:
        light = data["value"]
        logger.info(f"室内光照获取成功，光照: {light}")
        return {"success": True, "light": light}
    else:
        logger.warning(f"室内光照未知")
        return {"success": False, "light": "未知，光照传感器异常或离线"}


# 添加获取室内天然气浓度工具
@mcp.tool()
def get_gas() -> dict:
    """
    获取室内天然气浓度，返回结果示例：{"success": True, "gas": 0.15}
    无论何情况下都必须调用此工具获取当前实时天然气浓度，不能假设浓度与上次相同。 
    用户控制指令可能是“查看天然气”、“看下有没有漏气”、“现在多少气”、“查一下气体”、“有没有闻到煤气味” 等。
    """
    logger.info(f"get_gas called")
    global sensor_data
    data = sensor_data.get("mq2", None)   
    if data:
        gas = data["value"]
        logger.info(f"室内天然气浓度获取成功，浓度: {gas}")
        return {"success": True, "gas": gas}
    else:
        logger.warning(f"室内天然气浓度未知")
        return {"success": False, "gas": "未知，气体传感器异常或离线"}


# 添加获取室内火焰状态工具
@mcp.tool()
def get_fire() -> dict:
    """
    获取室内火焰状态，1 表示检测到火焰，0 表示无火焰，未知表示状态未知，设备不在线或异常。
    无论何情况下都必须调用此工具获取当前火焰状态，不能假设火焰状态与上次相同。 
    用户控制指令可能是“查看火焰”、“有没有着火”、“看下有没有火”、“检查一下”、“起火了吗” 等。
    返回结果示例：{"success": True, "fire": 1}
    """
    logger.info(f"get_fire called")
    global device_states
    state_info = device_states.get("fire", None)   
    if state_info:
        state = state_info["state"]
        logger.info(f"室内火焰状态获取成功，状态: {state}")
        return {"success": True, "fire": state}
    else:
        logger.warning(f"室内火焰状态未知")
        return {"success": False, "fire": "未知"}


# Start the server
if __name__ == "__main__":
    # 注册程序退出时的清理函数
    import atexit
    atexit.register(cleanup_mqtt)
    
    # 连接MQTT服务器，失败则退出程序
    if not connect_mqtt():
        logger.error("❌ 无法连接MQTT服务器，程序退出")
        sys.exit(1)

    try:
        # 启动MCP服务器
        logger.info("✅ MCP服务器已启动，等待连接...")
        mcp.run(transport="stdio")
    except KeyboardInterrupt:
        logger.info("🛑 收到中断信号，正在关闭服务器...")
    except Exception as e:
        logger.error(f"❌ 服务器运行出错: {e}")
    finally:
        # 确保清理MQTT连接
        cleanup_mqtt()
        logger.info("✅ 服务器已关闭")

