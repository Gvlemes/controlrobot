#include "mainwindow.h"
#include <QApplication>

// Librerías nativas de Qt6 para solicitar permisos dinámicos en Android
#include <QtCore/QCoreApplication>
#include <QPermissions>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ---- SOLICITUD DE PERMISOS REAL EN PANTALLA PARA CELULARES ----

    // 1. Solicitar Permiso de Escaneo de Dispositivos Cercanos (Bluetooth Scan)
    QBluetoothPermission permisoScan;
    qApp->requestPermission(permisoScan, [](const QPermission &permission) {
        if (permission.status() != Qt::PermissionStatus::Granted) {
            qWarning("Permiso de escaneo Bluetooth denegado por el usuario.");
        }
    });

    // 2. Solicitar Permiso de Ubicación Fina Estricta (Obligatorio por Android para ver Bluetooth Clásico/BLE)
    QLocationPermission permisoUbicacion;
    permisoUbicacion.setAccuracy(QLocationPermission::Precise);
    qApp->requestPermission(permisoUbicacion, [](const QPermission &permission) {
        if (permission.status() != Qt::PermissionStatus::Granted) {
            qWarning("Permiso de Ubicación denegado. No se detectarán robots.");
        }
    });

    // ---------------------------------------------------------------

    MainWindow w;
    w.show();
    return a.exec();
}
