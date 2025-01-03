#ifndef MODBUS_UTILS_HPP
#define MODBUS_UTILS_HPP

#include <modbus.h>
#include <iostream>
#include <errno.h>
#include <cstring>

modbus_t* initialize_modbus(const char* device);

#endif // MODBUS_UTILS_HPP
