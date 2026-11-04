import time  
import serial
import base64
import asyncio
import websockets
import json
import gc  # 引入垃圾回收模块
from maix import camera, display, image, nn, app, network
from config import (
    MODEL_PATH,
    UART_BAUD_RATE,
    UART_DEVICE,
    WEBSOCKET_HOST,
    WEBSOCKET_PORT,
    WIFI_PASSWORD,
    WIFI_SSID,
)

# ======================================================================
# 🚀 第 0 步：强制延时，避开系统底层开机日志风暴
# ======================================================================
print("等待 5 秒，跳过系统底层开机日志...")
time.sleep(5) 

# --- 1. 连接 WiFi ---
print("="*40)
print("📡 正在连接 WiFi...")
w = network.wifi.Wifi()
w.connect(WIFI_SSID, WIFI_PASSWORD)

my_ip = ""
for i in range(15):
    ip = w.get_ip()
    if ip and ip != "0.0.0.0":
        my_ip = ip
        break
    time.sleep(1)

if not my_ip:
    print("❌ WiFi 连接失败！")
    exit()

print(f"✅ WiFi 已连接！IP: {my_ip}")
print(f"🔗 小程序连接地址: ws://{my_ip}:{WEBSOCKET_PORT}")
print("="*40)

# --- 2. 硬件和模型初始化 ---
try:
    # 按照之前的约定，为了单片机稳定，这里波特率使用 9600
    uart = serial.Serial(UART_DEVICE, UART_BAUD_RATE, timeout=0.1)
    print(f"✨ 串口 {UART_DEVICE} ({UART_BAUD_RATE}) 初始化成功")
except Exception as e:
    uart = None
    print(f"❌ 串口打开失败: {e}")

# 加载 YOLO 模型
print("🧠 正在加载 AI 模型...")
detector = nn.YOLOv5(model=MODEL_PATH)
cam = camera.Camera(320, 240) 
disp = display.Display()

# --- 3. 共享数据盒子 ---
shared_data = {
    "jpg_b64": "", 
    "pet": "",
    "client_count": 0
}

# --- 4. 视觉识别主循环 (坦克级稳定版) ---
async def detection_loop():
    last_serial_time = 0
    frame_count = 0

    while True:
        try:
            # 1. 抓取图像
            img = cam.read()
            if img is None: 
                await asyncio.sleep(0.1) # 没抓到图就睡 100ms，防止死循环占满 CPU
                continue
            
            frame_count += 1
            
            # 2. 【大幅降维】：只在偶数帧进行图像压缩，画质降到 30
            if frame_count % 2 == 0:
                try:
                    clean_jpg_img = img.to_jpeg(quality=30) 
                    if clean_jpg_img is not None:
                        clean_jpg_bytes = clean_jpg_img.to_bytes()
                        shared_data["jpg_b64"] = base64.b64encode(clean_jpg_bytes).decode('utf-8')
                        del clean_jpg_bytes # 用完立刻销毁巨大变量
                    del clean_jpg_img
                except Exception as e:
                    print(f"⚠️ 图片压缩丢帧: {e}")

            # 3. AI 目标检测
            objs = detector.detect(img, conf_th=0.5, iou_th=0.45)
            
            target_detected = False
            pet_status_str = ""
            name_en = "NONE"
            status_en = "NONE"

            if objs: 
                for obj in objs:
                    # 15: 猫, 16: 狗
                    if obj.class_id == 15 or obj.class_id == 16:
                        target_detected = True
                        name_en = "CAT" if obj.class_id == 15 else "DOG"

                        # 姿态判断逻辑 (高度与宽度的比例)
                        aspect_ratio = obj.w / obj.h
                        center_y = obj.y + (obj.h / 2)
                        
                        if aspect_ratio > 1.2:
                            if center_y > 120:
                                status_en = "EATING"
                                pet_status_str = "CAT (EATING)"
                            else:
                                status_en = "RESTING"
                                pet_status_str = "CAT (RESTING)"
                        else:
                            status_en = "WAITING"
                            pet_status_str = "CAT (WAITING)"
                        
                        # 在屏幕上画框
                        img.draw_rect(obj.x, obj.y, obj.w, obj.h, color=image.COLOR_GREEN, thickness=2)
                        img.draw_string(obj.x, obj.y - 25, f"{name_en}:{status_en}", color=image.COLOR_GREEN, scale=1.2)
                        break 

            shared_data["pet"] = pet_status_str if target_detected else ""

            # 4. 【串口发送】：每秒严格只发一次，绝不轰炸单片机
            current_time = time.time()
            if current_time - last_serial_time >= 1.0:
                if target_detected:
                    send_msg = f"{name_en},{status_en}\r\n"
                else:
                    send_msg = "NONE,NONE\r\n"
                
                if uart: 
                    uart.write(send_msg.encode('utf-8'))
                
                # 控制台日志也一秒打印一次，防止刷屏卡死
                print(f"[{current_time:.1f}] 📡 发送 -> {send_msg.strip()}")
                last_serial_time = current_time

            # 5. 显示画面 (忽略 IDE 断开导致的报错)
            try:
                disp.show(img)
            except:
                pass 

            # =========================================================
            # 🛡️ 6. 终极内存保洁与降频调优
            # =========================================================
            del img
            del objs
            
            # 每 20 帧强制进行一次深度垃圾回收
            if frame_count % 20 == 0:
                gc.collect()

            # 强制休眠 0.1 秒！限制帧率在 10 FPS 以下，保证芯片绝对凉爽不崩溃！
            await asyncio.sleep(0.1)

        except Exception as loop_err:
            print(f"🔥 [护甲拦截] 视觉循环发生异常: {loop_err}")
            # 即使发生致命错误，也只是睡 0.5 秒，然后继续满血复活抓下一帧
            await asyncio.sleep(0.5)

# --- 5. WebSocket 视频流服务 ---
async def video_stream_server(websocket, path):
    shared_data["client_count"] += 1
    print(f"🔔 小程序连接成功! 当前连接数: {shared_data['client_count']}")

    try:
        while True:
            if shared_data["jpg_b64"]:
                package = {
                    "image": shared_data["jpg_b64"],
                    "pet": shared_data["pet"]
                }
                await websocket.send(json.dumps(package))
            # 视频流发送频率也降低到 10 FPS
            await asyncio.sleep(0.1)

    except websockets.exceptions.ConnectionClosed:
        shared_data["client_count"] -= 1
        print("❌ 小程序断开连接")
    except Exception as e:
        print(f"⚠️ 视频流传输异常: {e}")

# --- 6. 启动主程序 ---
async def main():
    await asyncio.sleep(1)
    # 启动 WebSocket 服务
    async with websockets.serve(
        video_stream_server,
        WEBSOCKET_HOST,
        WEBSOCKET_PORT,
    ):
        print("🚀 WebSocket 服务就绪，等待小程序连接...")
        # 启动视觉识别循环
        await detection_loop()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 程序已手动停止")
    except Exception as e:
        print(f"\n❌ 整个程序发生致命崩溃: {repr(e)}")
