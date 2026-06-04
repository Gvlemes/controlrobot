#define MAINWINDOW_H
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
    void transmitirComandoActivo(); // Nuevo método repetidor de ráfagas

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
    
    QTimer *relojRepetidorBluetooth; // El reloj que mantendrá vivo el pulso en tu celular
    char comandoActual;              // Almacena qué letra enviar ('F', 'B', 'L', 'R', 'S')
    QString urlFormateadaVideo;
};

#endif // MAINWINDOW_H
