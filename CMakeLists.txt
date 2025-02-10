cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MyWxApp)

# เพิ่มตรงส่วนบนของไฟล์หลัง project(MyWxApp)

# กำหนดไดเรกทอรีที่เก็บไฟล์ .cpp และ .hpp
include_directories( ./include )

# ตั้งค่าพาธสำหรับ vcpkg
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "E:/Year-4/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

# กำหนดพาธของ wxWidgets
set(wxWidgets_DIR "E:/Year-4/vcpkg/installed/x64-windows/share/wxwidgets")

# กำหนดพาธของ NanoSVG (ถ้ามีการใช้ NanoSVG)
set(NanoSVG_DIR "E:/Year-4/vcpkg/installed/x64-windows/share/nanosvg")

# ตั้งค่า CMake Policy
cmake_policy(SET CMP0167 NEW)

# ตั้งค่าพาธสำหรับ Boost
set(CMAKE_PREFIX_PATH "E:/Year-4/vcpkg/installed/x64-windows/share")

# ค้นหา wxWidgets
find_package(wxWidgets CONFIG REQUIRED)

# เพิ่ม dependencies
find_package(cppwinrt CONFIG REQUIRED)

# ค้นหา NanoSVG
find_package(NanoSVG REQUIRED)

# ค้นหา Boost (รวมถึง Boost.Asio และ Boost.System)
find_package(Boost REQUIRED COMPONENTS system asio)

# ค้นหา libmodbus
find_package(libmodbus CONFIG REQUIRED)

# ค้นหา Matplot++
find_package(Matplot++ CONFIG REQUIRED)

find_package(implot CONFIG REQUIRED)

# เพิ่มไฟล์ต้นฉบับ
add_executable(MyWxApp WIN32 main.cpp src/modbus_utils.cpp src/serial_utils.cpp src/comport_setting.cpp src/mainwindow_properties.cpp src/manual_calibrate.cpp src/mathplot.cpp)

set_target_properties(MyWxApp PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

# ลิงก์ wxWidgets, Boost และ libmodbus
target_link_libraries(MyWxApp 
    PRIVATE
        ${wxWidgets_LIBRARIES} 
        Boost::system 
        Boost::asio
        modbus
        Matplot++::cimg  
        Matplot++::matplot
        windowsapp
	    implot::implot
)

# เพิ่มการกำหนด include directories
target_include_directories(MyWxApp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# คัดลอกไฟล์จากไดเรกทอรี media ไปยังโฟลเดอร์ build
file(COPY ./media/ DESTINATION ${CMAKE_BINARY_DIR}/media)
