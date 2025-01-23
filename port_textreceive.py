import serial
import time
import struct

# การแปลงข้อมูลตามโครงสร้าง MEA_VALUE_STRUCTURE
def parse_mea_value_structure(data):
    # แปลงข้อมูลจาก hex เป็น byte
    data_byte = bytes.fromhex(data)

    # เรียง 4 ไบต์ในรูปแบบ Big Endian
    value_big_endian = data_byte[::-1]
    
    # แปลงเป็น float โดยใช้ Big Endian
    value_float = struct.unpack('>f', value_big_endian)[0]
    
 
    
    # แสดงผลค่า Value เท่านั้น
    result = {
        "Value (Float)": value_float,
     
    }
    return result

def send_and_receive_between_ports(port_send, baudrate=115200, timeout=1, parity='N', bytesize=8, stopbits=1):
    try:
        # เปิดการเชื่อมต่อกับ port_send (COM2) และ port_receive (COM1)
        ser_send = serial.Serial(port_send, baudrate, timeout=timeout, parity=parity, bytesize=bytesize, stopbits=stopbits)
        # ser_receive = serial.Serial(port_receive, baudrate, timeout=timeout, parity=parity, bytesize=bytesize, stopbits=stopbits)

        # ข้อมูลที่ต้องการส่ง
        test_message = bytes.fromhex("01090401010201ED00")
        print(f"Port {port_send} sending: {test_message.hex()}")

        # ส่งข้อมูลจาก COM2 ไปยัง COM1
        ser_send.write(test_message)
        ser_send.flush()  # ทำการ flush ข้อมูล
        # print(f"Sent data to {port_receive}: {test_message.hex()}")
        time.sleep(1)

        # # COM1 รับข้อมูลและตอบกลับ
        # response_message = b"" 
        # while True:
        #     chunk = ser_receive.read(ser_receive.in_waiting or 1)
        #     if chunk:
        #         response_message += chunk
        #     else:
        #         break

        # if response_message:
        #     print(f"Port {port_receive} received (hex): {response_message.hex()}")

        # อ่านข้อมูลที่ COM2 รับกลับจาก COM1
        reply_message = b"" 
        while True:

            chunk = ser_send.read(ser_send.in_waiting or 1)
            if chunk:
                reply_message += chunk
            else:
                break

        if reply_message:
            print(f"Port {port_send} received (reply message) (hex): {reply_message.hex()}")

        # การจัดการกับ format ที่ตอบกลับจาก COM1
        # ตรวจสอบ format ของข้อมูลก่อนที่จะนำไปแปลง
        data_hex = reply_message.hex()
        trimmed_data_hex = data_hex[6:-10]
        print(f"Trimmed data: {trimmed_data_hex}")
        # ตรวจสอบว่า reply_message มีรูปแบบที่ต้องการหรือไม่
        if len(data_hex) >= 8:  # ตรวจสอบความยาวของข้อมูลเพื่อให้แน่ใจว่าอยู่ในรูปแบบที่ถูกต้อง
            parsed_data = parse_mea_value_structure(trimmed_data_hex)
            for key, value in parsed_data.items():
                print(f"{key}: {value}")
        else:
            print("Received data does not have the expected format or is too short")

        # ปิดการเชื่อมต่อ
        ser_send.close()
        # ser_receive.close()

    except Exception as e:
        print(f"Error: {e}")

# ระบุชื่อพอร์ต
port_send = "COM2"
# port_receive = "COM8"

while True:
    try:
        # เรียกใช้ฟังก์ชัน
        send_and_receive_between_ports(port_send) 
    except Exception as e:
        print(f"Error: {e}")
        break
