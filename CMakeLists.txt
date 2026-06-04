cmake_minimum_required(VERSION 3.16)
project(RobotControlPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Bluetooth Network)

add_executable(RobotControlPro 
    main.cpp 
    mainwindow.cpp 
    mainwindow.h
)

target_link_libraries(RobotControlPro PRIVATE 
    Qt6::Widgets 
    Qt6::Bluetooth
    Qt6::Network
)

if(ANDROID)
    set_property(TARGET RobotControlPro PROPERTY QT_ANDROID_SCREEN_ORIENTATION landscape)

    # 🔓 SOLUCIÓN AL ERROR DE CÁMARA: Forzar a Android a permitir tráfico HTTP de IP Webcam
    set_property(TARGET RobotControlPro APPEND PROPERTY QT_ANDROID_MANIFEST_EXTENSIONS
        "android:usesCleartextTraffic=\"true\""
    )

    set_property(TARGET RobotControlPro APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.INTERNET
        android.permission.BLUETOOTH_SCAN
        android.permission.BLUETOOTH_CONNECT
        android.permission.ACCESS_FINE_LOCATION
        android.permission.ACCESS_COARSE_LOCATION
    )

    qt_finalize_target(RobotControlPro)
endif()
