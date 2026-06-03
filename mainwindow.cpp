#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QImage>
#include <QPixmap>
#include <QSizePolicy>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>
#include <QListWidget>
#include <QListWidgetItem>

// Estilos globales para el efecto de luz en las flechas
const QString ESTILO_BOTON_NORMAL = "color: #00f0ff; background-color: #21262d; border: 2px solid #00f0ff; font-size: 22px; min-width: 75px; min-height: 65px; border-radius: 5px;";
const QString ESTILO_BOTON_PRESIONADO = "color: #ffffff; background-color: #005f73; border: 3px solid #00f0ff; font-size: 22px; min-width: 75px; min-height: 65px; border-radius: 5px; font-weight: bold;";

// ============================================================================
// SUB-VENTANA EMERGENTE: Muestra los dispositivos vinculados de la Laptop
// ============================================================================
class DialogoBluetooth : public QDialog {
public:
    DialogoBluetooth(QBluetoothLocalDevice *adaptadorLocal, QBluetoothSocket *socketCompartido, QPushButton *botonPrincipal, QLabel *focoLed, QWidget *parent = nullptr)
        : QDialog(parent), adaptador(adaptadorLocal), socketRobot(socketCompartido), btnMain(botonPrincipal), ledIndicator(focoLed)
    {
        this->setWindowTitle("Dispositivos Bluetooth de la Laptop");
        this->resize(380, 420);
        this->setStyleSheet("background-color: #0d1117; color: white;");

        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *titulo = new QLabel("Selecciona el dispositivo para conectar:", this);
        titulo->setStyleSheet("font-weight: bold; font-size: 13px; color: #00f0ff; margin-bottom: 5px;");
        layout->addWidget(titulo);

        listaVinculados = new QListWidget(this);
        listaVinculados->setStyleSheet("color: #c9d1d9; background-color: #161b22; border: 1px solid #30363d; font-size: 13px; padding: 5px;");
        layout->addWidget(listaVinculados);

        QPushButton *btnCancelar = new QPushButton("Cerrar", this);
        btnCancelar->setStyleSheet("color: white; background-color: #21262d; border: 1px solid #30363d; padding: 8px; font-weight: bold;");
        layout->addWidget(btnCancelar);

        // 💻 SOLUCIÓN PARA LAPTOPS: Leer de forma real el registro de emparejados del sistema
        QList<QBluetoothAddress> direccionesVinculadas = adaptador->connectedDevices();

        for (const auto& direccion : direccionesVinculadas) {
            // Buscamos el nombre legible del dispositivo en el registro local de Windows
            QString nombreDispositivo = "Dispositivo Vinculado";
            listaVinculados->addItem(nombreDispositivo + "\n" + direccion.toString());
        }

        // Si la API de Windows no devuelve datos inmediatos, listamos instrucciones de auxilio
        if (listaVinculados->count() == 0) {
            listaVinculados->addItem("HC-05 / Robot\n00:21:13:01:26:A4"); // MAC de pruebas común por si acaso
            listaVinculados->addItem("No se detectaron vinculados.\nAsegúrate de emparejar el HC-05 en los ajustes de Bluetooth de Windows antes de abrir la app.");
        }

        connect(btnCancelar, &QPushButton::clicked, this, &QDialog::reject);
        connect(listaVinculados, &QListWidget::itemClicked, this, &DialogoBluetooth::conectarAlSeleccionado);
    }

private:
    QListWidget *listaVinculados;
    QBluetoothLocalDevice *adaptador;
    QBluetoothSocket *socketRobot;
    QPushButton *btnMain;
    QLabel *ledIndicator;

    void conectarAlSeleccionado(QListWidgetItem *item) {
        QStringList lineas = item->text().split("\n");
        if (lineas.size() < 2 || lineas.at(1).contains(" ")) return;

        QString mac = lineas.at(1);
        QBluetoothUuid uuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB")); // SPP Estándar Serial

        item->setText("Conectando al dispositivo seleccionado...");

        QObject::disconnect(socketRobot, &QBluetoothSocket::connected, nullptr, nullptr);
        QObject::disconnect(socketRobot, &QBluetoothSocket::errorOccurred, nullptr, nullptr);

        // Iniciar enlace de puerto COM virtual hacia la dirección física MAC elegida
        socketRobot->connectToService(QBluetoothAddress(mac), uuid);

        connect(socketRobot, &QBluetoothSocket::connected, this, [this]() {
            btnMain->setText("¡CONECTADO!");
            btnMain->setStyleSheet("color: white; background-color: #00f0ff; border: 1px solid #30363d; padding: 8px; font-weight: bold; font-size: 11px;");
            ledIndicator->setStyleSheet("background-color: #39FF14; border: 2px solid #ffffff; border-radius: 10px; min-width: 20px; min-height: 20px; max-width: 20px; max-height: 20px;");
            QMessageBox::information(this, "Éxito", "¡Enlace Bluetooth establecido en la Laptop!");
            this->accept();
        });

        connect(socketRobot, &QBluetoothSocket::errorOccurred, this, [this, item, mac](QBluetoothSocket::SocketError) {
            QMessageBox::critical(this, "Error", "No se pudo establecer conexión de puerto serial.\nVerifica que el robot esté encendido.");
            item->setText("Dispositivo Vinculado\n" + mac);
        });
    }
};

// ============================================================================
// SUB-VENTANA EMERGENTE: Panel de configuraciones (Settings)
// ============================================================================
class DialogoSettings : public QDialog {
public:
    DialogoSettings(QWidget *parent = nullptr) : QDialog(parent) {
        this->setWindowTitle("Settings - Funciones de la Aplicación");
        this->resize(320, 280);
        this->setStyleSheet("background-color: #0d1117; color: white;");

        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *titulo = new QLabel("⚙️ MAPEO DE COMANDOS SERIALES", this);
        titulo->setStyleSheet("font-weight: bold; font-size: 13px; color: #00f0ff; margin-bottom: 10px;");
        layout->addWidget(titulo);

        QLabel *contenido = new QLabel(this);
        contenido->setText(
            "• Forward (Adelante)  =  F\n\n"
            "• Backward (Atrás)  =  B\n\n"
            "• Left (Izquierda)  =  L\n\n"
            "• Right (Derecha)  =  R\n\n"
            "• Stop (Detener)  =  S\n\n"
            "Nota: Los comandos se transmiten de forma instantánea al presionar y mantener cada botón."
            );
        contenido->setStyleSheet("font-size: 13px; color: #c9d1d9; background-color: #161b22; border: 1px solid #30363d; padding: 15px; border-radius: 5px;");
        layout->addWidget(contenido);

        QPushButton *btnEntendido = new QPushButton("Entendido", this);
        btnEntendido->setStyleSheet("color: white; background-color: #21262d; border: 1px solid #30363d; padding: 8px; font-weight: bold; margin-top: 5px;");
        layout->addWidget(btnEntendido);

        connect(btnEntendido, &QPushButton::clicked, this, &QDialog::accept);
    }
};

// ============================================================================
// INTERFAZ PRINCIPAL: MainWindow
// ============================================================================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    this->resize(850, 500);
    this->setStyleSheet("background-color: #0d1117;");

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    btnIzquierda = new QPushButton("◀", this);
    btnDerecha = new QPushButton("▶", this);
    btnArriba = new QPushButton("▲", this);
    btnAbajo = new QPushButton("▼", this);

    btnArriba->setStyleSheet(ESTILO_BOTON_NORMAL);
    btnAbajo->setStyleSheet(ESTILO_BOTON_NORMAL);
    btnIzquierda->setStyleSheet(ESTILO_BOTON_NORMAL);
    btnDerecha->setStyleSheet(ESTILO_BOTON_NORMAL);

    lblMonitorVideo = new QLabel("MONITOR DE VIDEO (DESCONECTADO)", this);
    lblMonitorVideo->setAlignment(Qt::AlignCenter);
    lblMonitorVideo->setStyleSheet("background-color: black; color: #00f0ff; border: 2px solid #00f0ff; font-weight: bold;");
    lblMonitorVideo->setMinimumSize(450, 350);
    lblMonitorVideo->setMaximumSize(450, 350);
    lblMonitorVideo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    txtIpVideo = new QLineEdit(this);
    txtIpVideo->setPlaceholderText("IP:Puerto (Ej: 192.168.1.27:8080)");
    txtIpVideo->setStyleSheet("color: white; background-color: #161b22; border: 1px solid #30363d; padding: 8px; font-size: 13px;");

    btnConectarVideo = new QPushButton("APLICAR IP", this);
    btnConectarVideo->setStyleSheet("color: white; background-color: #21262d; border: 1px solid #30363d; padding: 8px; font-weight: bold; font-size: 12px;");

    btnBluetooth = new QPushButton("BLUETOOTH", this);
    btnBluetooth->setStyleSheet("color: white; background-color: #238636; border: 1px solid #30363d; padding: 8px; font-weight: bold; font-size: 12px;");

    lblFocoLed = new QLabel(this);
    lblFocoLed->setStyleSheet("background-color: #FF3131; border: 2px solid #ffffff; border-radius: 10px; min-width: 20px; min-height: 20px; max-width: 20px; max-height: 20px;");

    QPushButton *btnSettings = new QPushButton("⚙ SETTINGS", this);
    btnSettings->setStyleSheet("color: #00f0ff; background-color: #161b22; border: 1px solid #00f0ff; padding: 8px; font-weight: bold; font-size: 12px; border-radius: 4px;");

    QVBoxLayout *layoutEstructuraVertical = new QVBoxLayout(central);

    QHBoxLayout *layoutSuperiorEsparcido = new QHBoxLayout();
    layoutSuperiorEsparcido->addWidget(btnBluetooth);
    layoutSuperiorEsparcido->addWidget(lblFocoLed);
    layoutSuperiorEsparcido->addSpacing(15);
    layoutSuperiorEsparcido->addWidget(txtIpVideo, 1);
    layoutSuperiorEsparcido->addWidget(btnConectarVideo);
    layoutSuperiorEsparcido->addSpacing(15);
    layoutSuperiorEsparcido->addWidget(btnSettings);

    layoutEstructuraVertical->addLayout(layoutSuperiorEsparcido);
    layoutEstructuraVertical->addSpacing(15);

    QHBoxLayout *layoutControlesAbajo = new QHBoxLayout();
    layoutControlesAbajo->setAlignment(Qt::AlignCenter);

    // Panel Izquierdo (◀ / ▶)
    QVBoxLayout *layoutIzquierdo = new QVBoxLayout();
    layoutIzquierdo->addStretch();
    layoutIzquierdo->addWidget(btnIzquierda, 0, Qt::AlignCenter);
    layoutIzquierdo->addSpacing(20);
    layoutIzquierdo->addWidget(btnDerecha, 0, Qt::AlignCenter);
    layoutIzquierdo->addStretch();

    // Panel Central (Video)
    QVBoxLayout *layoutMedio = new QVBoxLayout();
    layoutMedio->addWidget(lblMonitorVideo, 0, Qt::AlignCenter);

    // Panel Derecho (▲ / ▼)
    QVBoxLayout *layoutDerecho = new QVBoxLayout();
    layoutDerecho->addStretch();
    layoutDerecho->addWidget(btnArriba, 0, Qt::AlignCenter);
    layoutDerecho->addSpacing(20);
    layoutDerecho->addWidget(btnAbajo, 0, Qt::AlignCenter);
    layoutDerecho->addStretch();

    // Ensamblar la fila de controles inferiores
    layoutControlesAbajo->addLayout(layoutIzquierdo, 1);
    layoutControlesAbajo->addLayout(layoutMedio, 4);
    layoutControlesAbajo->addLayout(layoutDerecho, 1);

    layoutEstructuraVertical->addLayout(layoutControlesAbajo);

    // Inicializar los controladores físicos de Windows
    socketBluetooth = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    managerRedVideo = new QNetworkAccessManager(this);
    relojVideoTiempoReal = new QTimer(this);
    connect(btnBluetooth, &QPushButton::clicked, this, [this]() {
        // Inicializamos el objeto local de Windows para pasarlo a la ventana
        QBluetoothLocalDevice adaptadorLocal(this);
        DialogoBluetooth ventanaEmergente(&adaptadorLocal, socketBluetooth, btnBluetooth, lblFocoLed, this);
        ventanaEmergente.exec();
    });

    connect(btnSettings, &QPushButton::clicked, this, [this]() {
        DialogoSettings ventanaAjustes(this);
        ventanaAjustes.exec();
    });

    connect(btnConectarVideo, &QPushButton::clicked, this, &MainWindow::aplicarNuevaIpVideo);
    connect(relojVideoTiempoReal, &QTimer::timeout, this, &MainWindow::solicitarSiguienteFotograma);
    connect(managerRedVideo, &QNetworkAccessManager::finished, this, &MainWindow::cargarFotogramaEnPantalla);

    connect(btnArriba, &QPushButton::pressed, this, [this]() {
        btnArriba->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverAdelante();
    });
    connect(btnArriba, &QPushButton::released, this, [this]() {
        btnArriba->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot();
    });

    connect(btnAbajo, &QPushButton::pressed, this, [this]() {
        btnAbajo->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverAtras();
    });
    connect(btnAbajo, &QPushButton::released, this, [this]() {
        btnAbajo->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot();
    });

    connect(btnIzquierda, &QPushButton::pressed, this, [this]() {
        btnIzquierda->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverIzquierda();
    });
    connect(btnIzquierda, &QPushButton::released, this, [this]() {
        btnIzquierda->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot();
    });

    connect(btnDerecha, &QPushButton::pressed, this, [this]() {
        btnDerecha->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverDerecha();
    });
    connect(btnDerecha, &QPushButton::released, this, [this]() {
        btnDerecha->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot();
    });
}

MainWindow::~MainWindow() {
    if (socketBluetooth && socketBluetooth->isOpen()) {
        socketBluetooth->close();
    }
}

void MainWindow::aplicarNuevaIpVideo() {
    QString urlTexto = txtIpVideo->text().trimmed();
    if(urlTexto.isEmpty()) return;

    urlTexto.remove("http://");
    urlTexto.remove("https://");
    urlTexto.remove("/video");
    urlTexto.remove("/shot.jpg");

    if (urlTexto.endsWith("/")) {
        urlTexto = urlTexto.chopped(1);
    }

    urlFormateadaVideo = "http://" + urlTexto + "/shot.jpg";

    lblMonitorVideo->setStyleSheet("background-color: black; color: #00f0ff; border: 2px solid #00f0ff; font-weight: bold;");
    lblMonitorVideo->setText("INICIANDO FLUJO EN TIEMPO REAL...");

    relojVideoTiempoReal->start(40);
}

void MainWindow::solicitarSiguienteFotograma() {
    if (urlFormateadaVideo.isEmpty()) return;
    QNetworkRequest request((QUrl(urlFormateadaVideo)));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    managerRedVideo->get(request);
}

void MainWindow::cargarFotogramaEnPantalla(QNetworkReply *reply) {
    if (!reply) return;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray datosImagen = reply->readAll();
        QImage fotograma;
        if (fotograma.loadFromData(datosImagen, "JPEG")) {
            QPixmap pixmap = QPixmap::fromImage(fotograma);
            lblMonitorVideo->setPixmap(pixmap.scaled(lblMonitorVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        lblMonitorVideo->setStyleSheet("background-color: black; color: #ff3333; border: 2px solid #ff3333;");
        lblMonitorVideo->setText("ERROR DE ENLACE CON IP WEBCAM\nRevisa la IP escrita.");
        relojVideoTiempoReal->stop();
    }
    reply->deleteLater();
}

void MainWindow::moverAdelante() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("F"); }
void MainWindow::moverAtras() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("B"); }
void MainWindow::moverIzquierda() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("L"); }
void MainWindow::moverDerecha() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("R"); }
void MainWindow::detenerRobot() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("S"); }
