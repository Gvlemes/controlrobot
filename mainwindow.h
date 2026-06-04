#ifndef MAINWINDOW_H    // <-- CORREGIDO: Ahora el #endif final tiene su pareja correspondiente
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QBluetoothSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void aplicarNuevaIpVideo();
    void solicitarSiguienteFotograma();
    void cargarFotogramaEnPantalla(QNetworkReply *reply);
    void transmitirComandoActivo(); 

    void moverAdelante();
    void moverAtras();
    void moverIzquierda();
    void moverDerecha();
    void detenerRobot();

private:
    QLineEdit *txtIpVideo;
    QPushButton *btnConectarVideo;
    QLabel *lblMonitorVideo;
    
    QPushButton *btnBluetooth;
    QLabel *lblFocoLed;
    
    QPushButton *btnArriba;
    QPushButton *btnAbajo;
    QPushButton *btnIzquierda;
    QPushButton *btnDerecha;

    QBluetoothSocket *socketBluetooth;
    QNetworkAccessManager *managerRedVideo;
    QTimer *relojVideoTiempoReal;
    
    QTimer *relojRepetidorBluetooth; 
    char comandoActual;              
    QString urlFormateadaVideo;
};

#endif // MAINWINDOW_H
