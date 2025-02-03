#include "serial_utils.hpp"
#include <wx/wx.h>
using namespace std;
using namespace boost::asio;
//----------------------------------------------------------------------------------------------------------------------------------
// ฟังก์ชันแปลงข้อมูลที่ได้รับจาก Serial Port ให้เป็นค่าที่เข้าใจได้
float parseMeaValueStructure(const string& data) {
    // แปลง Hex เป็น Byte Array
    vector<uint8_t> bytes;
    for (size_t i = 0; i < data.size(); i += 2) {
        uint8_t byte = std::stoi(data.substr(i, 2), nullptr, 16);
        bytes.push_back(byte);
    }
    // ใช้ลำดับไบต์ที่ถูกต้องตาม Big Endian
    // แปลง Byte Array เป็น Float โดยตรง (ไม่ reverse)
    float value;
    std::memcpy(&value, bytes.data(), sizeof(value));
    return value;
}
uint32_t hexToUint32(const std::string& hexStr) {
    // ตรวจสอบให้แน่ใจว่า hexStr มีความยาว 8 ตัวอักษร (4 ไบต์)
    if (hexStr.length() != 8) {
        throw std::invalid_argument("Invalid hex length");
    }
    // สลับไบต์จาก Big Endian เป็น Little Endian
    std::string reorderedHex = { hexStr[6], hexStr[7], hexStr[4], hexStr[5],
                                         hexStr[2], hexStr[3], hexStr[0], hexStr[1] };
    // แปลงค่า Hex เป็น uint32_t
    uint32_t result;
    std::stringstream ss;
    ss << std::hex << reorderedHex;
    ss >> result;
    return result;
}
//----------------------------------------------------------------------------------------------------------------------------------
uint32_t sendRequestSerialNumber(const string& portSend, unsigned int baudrate) {
    try {
        io_context io;
        serial_port serialSend(io, portSend);
        serialSend.set_option(serial_port_base::baud_rate(baudrate));
        serialSend.set_option(serial_port_base::character_size(8));
        // ข้อมูลที่ต้องการส่ง
        vector<uint8_t> requestSerialMessage = { 0x01, 0x07, 0x01, 0xFF, 0x03, 0xF5, 0x00 };
        // ส่งข้อมูล
        write(serialSend, buffer(requestSerialMessage));
        vector<uint8_t> responseBuffer(9); 
        read(serialSend, buffer(responseBuffer));
        // แสดงผลข้อมูลที่ได้รับ
        if (!responseBuffer.empty()) {
            std::ostringstream trimmedHex;
            for (size_t i = 3; i <= 6; ++i) {
                trimmedHex << hex << setfill('0') << setw(2) << (int)responseBuffer[i];
            }
            uint32_t uint32Value = hexToUint32(trimmedHex.str());
			return uint32Value;
        }
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 0;
    }
}
// ฟังก์ชันส่งและรับข้อมูลระหว่างพอร์ต
float sendAndReceiveBetweenPorts(const string& portSend, unsigned int baudrate) {
    try {
        io_context io;
        serial_port serialSend(io, portSend);

        serialSend.set_option(serial_port_base::baud_rate(baudrate));
        serialSend.set_option(serial_port_base::character_size(8));

        // ข้อมูลที่ต้องการส่ง
        vector<uint8_t> requestDataMessage = { 0x01, 0x09, 0x04, 0x01, 0x01, 0x02, 0x01, 0xED, 0x00 };
        // ส่งข้อมูล
        write(serialSend, buffer(requestDataMessage));
        vector<uint8_t> responseBuffer(12); // ต้องการอ่านให้ครบ 12 ไบต์
        read(serialSend, buffer(responseBuffer));
        // แสดงผลข้อมูลที่ได้รับ
        if (!responseBuffer.empty()) {
            std::ostringstream trimmedHex;
            for (size_t i = 3; i < responseBuffer.size() - 5; ++i) {
                trimmedHex << hex << setfill('0') << setw(2) << (int)responseBuffer[i];
            }
            float parsedValue = parseMeaValueStructure(trimmedHex.str());
            parsedValue = std::round(parsedValue * 1000.0f) / 1000.0f;
            return parsedValue;
        }else {
                cout << "No data received from " << portSend << "." << endl;
                return 0;
            }
            
    }catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 0;
    }
}
// ส่งคำสั่งไปยัง Serial Port
void send_scpi_command(serial_port& serial, const string& command, string& response, bool expect_response ) {
    if (!serial.is_open()) {
        // cerr << "Serial port is not open!" << endl;
        return;
    }
    // ส่งคำสั่งไปยัง Serial Port
    write(serial, buffer(command + "\n"));

    // ถ้าคำสั่งไม่ต้องการ response ให้หยุดที่นี่
    if (!expect_response) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //cout << "Command sent: " << command << endl;
        return;
    }
    // ถ้าคำสั่งต้องการ response
    boost::asio::streambuf buffer; //กำหนด namespace แบบเต็มเพื่อไม่ให้ compiler กำกวมกับการเลือกใช้ * ให้ใช้ class streambuf ของ boost *
    read_until(serial, buffer, '\n');
    // แปลงข้อมูลที่อ่านได้จาก buffer เป็น string
    istream input_stream(&buffer);
    getline(input_stream, response);
    // เช็คว่ามีข้อมูลหรือไม่
    if (!response.empty()) {
        //cout << "Received response: " << response << endl;
    } else {
        // cerr << "No data received or read error" << endl;
    }
}
void set_voltage(serial_port& serial, double voltage) {
    string command = "VOLT " + to_string(voltage);
    string response;
    send_scpi_command(serial, command, response , false);
    // cout << "Set Voltage to: " << voltage << " V" << endl;
}
void set_current(serial_port& serial, double current) {
    string command = "CURR " + to_string(current);
    string response;
    send_scpi_command(serial, command, response , false);
    // cout << "Set Current to: " << current << " A" << endl;
}