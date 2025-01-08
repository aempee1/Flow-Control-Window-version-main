from bleak import BleakScanner
import asyncio

async def fetch_nearby_ble_continuously():
    print("Starting BLE device scan... (Press Ctrl+C to stop)")
    try:
        while True:
            devices = await BleakScanner.discover()
            print("\nNearby BLE devices with names:")
            for device in devices:
                # ตรวจสอบว่าอุปกรณ์มีชื่อหรือไม่
                if device.name:  # ชื่อที่ไม่ใช่ None หรือค่าว่าง
                    print(f"Name: {device.name}, Address: {device.address}, RSSI: {device.rssi} dBm")
            # หน่วงเวลาเล็กน้อยก่อนสแกนรอบถัดไป
            await asyncio.sleep(2)  # ปรับเวลาให้เหมาะสม (เช่น 5 วินาที)
    except KeyboardInterrupt:
        print("\nScan stopped by user.")

# เรียกใช้งาน
asyncio.run(fetch_nearby_ble_continuously())
