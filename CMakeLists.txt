cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(MyWxApp)
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
#-------------------------------------------------------------------------------------------------
find_package(wxWidgets CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(NanoSVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system asio)
find_package(libmodbus CONFIG REQUIRED)
find_package(Matplot++ CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
#-------------------------------------------------------------------------------------------------
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
#-------------------------------------------------------------------------------------------------
add_executable(MyWxApp WIN32 main.cpp src/modbus_utils.cpp src/serial_utils.cpp src/comport_setting.cpp src/mainwindow_properties.cpp src/manual_calibrate.cpp src/mathplot.cpp)
#-------------------------------------------------------------------------------------------------
set_target_properties(MyWxApp PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
#-------------------------------------------------------------------------------------------------
target_link_libraries(
    MyWxApp 
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
target_include_directories(
    MyWxApp 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
file(COPY ./media/ DESTINATION ${CMAKE_BINARY_DIR}/media)
#-------------------------------------------------------------------------------------------------
add_custom_command(
    TARGET MyWxApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   $<TARGET_RUNTIME_DLLS:MyWxApp> $<TARGET_FILE_DIR:MyWxApp>
                   COMMAND_EXPAND_LISTS
)
#-------------------------------------------------------------------------------------------------
# เปิดใช้งาน CPack สำหรับสร้าง Installer
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "MyWxApp")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Suto Itec Thailand")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MyWxApp - An application built with wxWidgets")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MyWxApp")
# เพิ่ม Visual C++ Redistributable ถ้าต้องใช้
install(FILES "E:/Download/Documents/VC_redist.x64.exe" DESTINATION .)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR/vc_redist.x64.exe\\\" /restart'")
# สร้าง Shortcut บน Start Menu
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut \\\"$SMPROGRAMS\\\\MyWxApp.lnk\\\" \\\"$INSTDIR\\\\MyWxApp.exe\\\"")
# สร้างไฟล์ Uninstaller
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
# เพิ่มไฟล์ที่ต้องติดตั้ง
install(TARGETS MyWxApp DESTINATION .)
# install(FILES "E:/Year-4/vcpkg/installed/x64-windows/bin/wxbase32u_vc_x64_custom.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/boost_system-vc143-mt-x64-1_86.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/wxmsw32u_core_vc_x64_custom.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/modbus.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/jpeg62.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/libpng16.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/tiff.dll"
#                 "E:/Year-4/vcpkg/installed/x64-windows/bin/zlib1.dll"
#         DESTINATION .)
install(DIRECTORY "E:/Year-4/vcpkg/installed/x64-windows/bin/" DESTINATION .)


install(DIRECTORY media/ DESTINATION media)
#------------------------------------------------------------------------------------------------
include(CPack)
