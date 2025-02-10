import serial
import time

def send_data_from_port2_to_port1(port_send, baudrate=115200, timeout=1):
    try:
        # เปิดการเชื่อมต่อกับ port_send (Port 2) ที่จะส่งข้อมูล
        ser_send = serial.Serial(port_send, baudrate, timeout=timeout)

        # ข้อมูลที่ต้องการส่ง (ในที่นี้เราจะส่งเลขฐาน 16 '39')
        test_message = bytes.fromhex("39")
        print(f"Port {port_send} sending: {test_message.hex()}")

        # ส่งข้อมูลไปยัง port_send (Port 2) ที่จะไปยัง Port 1
        ser_send.write(test_message)
        time.sleep(1)  # รอให้ข้อมูลถูกส่ง

        # ปิดการเชื่อมต่อ
        ser_send.close()

    except Exception as e:
        print(f"Error: {e}")

# ระบุชื่อพอร์ตที่ใช้งาน
port_send = "COM2"  # แทนที่ด้วยชื่อพอร์ตที่คุณใช้ในการส่งข้อมูล (Port 2)

# เรียกใช้งานฟังก์ชัน
send_data_from_port2_to_port1(port_send)
