#ifndef SERIAL_UTILS_HPP
#define SERIAL_UTILS_HPP

#include <string>
#include <boost/asio.hpp>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <exception>


using namespace std;
using namespace boost::asio;

float parseMeaValueStructure(const string& data);
uint32_t hexToUint32(const std::string& hexStr);
void send_scpi_command(serial_port& serial, const string& command, string& response , bool expect_response = true);
float sendAndReceiveBetweenPorts(serial_port& serial);
uint32_t sendRequestSerialNumber(serial_port& serial);
void set_voltage(serial_port& serial, double voltage);
void set_current(serial_port& serial, double current);



#endif // SERIAL_UTILS_HPP