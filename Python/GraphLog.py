import matplotlib.pyplot as plt
import time

# เส้นที่ใช้แสดงผล
time_data = []  # เก็บเวลา
refFlow_data = []  # เก็บค่าของ refFlow
actFlow_data = []  # เก็บค่าของ actFlow

fifo_path = "/tmp/myfifo"

# สร้างกราฟเริ่มต้น
plt.ion()  # เปิดโหมด interactive plotting
fig, ax = plt.subplots()
ax.set_xlabel("Time (s)")  # แกน x คือ เวลา
ax.set_ylabel("Flow Speed (L/min)")  # แกน y คือ ความเร็วลท
ax.set_title("Real-Time Flow Speed")
ref_line, = ax.plot([], [], 'r-', label="refFlow")  # เส้นสีแดง
act_line, = ax.plot([], [], 'b-', label="actFlow")  # เส้นสีน้ำเงิน
ax.legend()

# เริ่มอ่านจาก FIFO และพล็อตกราฟ
start_time = time.time()  # เริ่มเวลา

try:
    with open(fifo_path, "r") as fifo:
        while True:
            data = fifo.readline().strip()
            if data:  # ถ้ามีข้อมูล
                time_elapsed = time.time() - start_time  # คำนวณเวลา
                refFlow, actFlow = map(float, data.split())  # แยกค่าจากกันและแปลงเป็น float

                # เพิ่มข้อมูลลงใน list
                time_data.append(time_elapsed)
                refFlow_data.append(refFlow)
                actFlow_data.append(actFlow)

                # อัพเดทกราฟ
                ref_line.set_data(time_data, refFlow_data)
                act_line.set_data(time_data, actFlow_data)

                # ปรับแกน x และ y
                ax.set_xlim(min(time_data), max(time_data) + 1)
                ax.set_ylim(min(min(refFlow_data), min(actFlow_data)) - 1, max(max(refFlow_data), max(actFlow_data)) + 1)

                plt.pause(0.2)  # เว้นระยะเวลา 200 ms
except KeyboardInterrupt:
    print("Stopped by user")
finally:
    plt.show()
