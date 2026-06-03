cmake_minimum_required(VERSION 3.16)
project(RobotControlPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Bluetooth Network)

# ⚠️ CORRECCIÓN CRÍTICA: Se cambió 'add_executable' por 'qt_add_executable'
# Esto le avisa a Qt 6 que prepare el proyecto de forma nativa para celulares
qt_add_executable(RobotControlPro
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

set_target_properties(RobotControlPro PROPERTIES
   QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")

target_link_libraries(RobotControlPro PRIVATE
    Qt6::Widgets
    Qt6::Bluetooth
    Qt6::Network
)

# Configuración limpia de permisos compatible con laptops y celulares
if(ANDROID)
    # 1. Forzar orientación de pantalla horizontal fija para Android
    set_property(TARGET RobotControlPro PROPERTY QT_ANDROID_SCREEN_ORIENTATION landscape)

    # 2. Inyección directa de permisos en las propiedades del ejecutable
    set_property(TARGET RobotControlPro APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.INTERNET
        android.permission.BLUETOOTH_SCAN
        android.permission.BLUETOOTH_CONNECT
        android.permission.ACCESS_FINE_LOCATION
        android.permission.ACCESS_COARSE_LOCATION
    )

    # 3. Empaquetador final del APK (Ahora sí funcionará porque el target es válido)
    qt_finalize_target(RobotControlPro)
endif()
