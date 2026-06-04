cmake_minimum_required(VERSION 3.16)
project(RobotControlPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Buscar los componentes necesarios de Qt6 (Widgets, Bluetooth y Red)
find_package(Qt6 REQUIRED COMPONENTS Widgets Bluetooth Network)

# 🛠️ CORRECCIÓN CRÍTICA: Cambiado 'add_executable' por 'qt_add_executable' para que sea válido en Android
qt_add_executable(RobotControlPro 
    main.cpp 
    mainwindow.cpp 
    mainwindow.h
)

# Vincular las librerías a tu aplicación del robot
target_link_libraries(RobotControlPro PRIVATE 
    Qt6::Widgets 
    Qt6::Bluetooth
    Qt6::Network
)

if(ANDROID)
    # Configurar orientación de la pantalla del robot
    set_property(TARGET RobotControlPro PROPERTY QT_ANDROID_SCREEN_ORIENTATION landscape)

    # Permite conexiones HTTP sin cifrar (ideal para cámaras IP y comandos directos de red)
    set_property(TARGET RobotControlPro APPEND PROPERTY QT_ANDROID_MANIFEST_EXTENSIONS
        "android:usesCleartextTraffic=\"true\""
    )

    # Inyección automática de todos los permisos necesarios en el AndroidManifest.xml
    set_property(TARGET RobotControlPro APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.INTERNET
        android.permission.BLUETOOTH_SCAN
        android.permission.BLUETOOTH_CONNECT
        android.permission.ACCESS_FINE_LOCATION
        android.permission.ACCESS_COARSE_LOCATION
    )

    # Finalizar el target para generar correctamente los metadatos del APK
    qt_finalize_target(RobotControlPro)
endif()
