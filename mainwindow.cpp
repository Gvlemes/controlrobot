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
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QListWidget>
#include <QListWidgetItem>

// Estilos globales para la interfaz oscura y efecto de luz neón
const QString ESTILO_BOTON_NORMAL = "color: #00f0ff; background-color: #21262d; border: 2px solid #00f0ff; font-size: 22px; min-width: 75px; min-height: 65px; border-radius: 5px;";
const QString ESTILO_BOTON_PRESIONADO = "color: #ffffff; background-color: #005f73; border: 3px solid #00f0ff; font-size: 22px; min-width: 75px; min-height: 65px; border-radius: 5px; font-weight: bold;";

// ============================================================================
// SUB-VENTANA EMERGENTE: Ventana Avanzada de Escaneo y Conexión Manual Directa
// ============================================================================
class DialogoBluetooth : public QDialog {
public:
    DialogoBluetooth(QBluetoothLocalDevice *adaptadorLocal, QBluetoothSocket *socketCompartido, QPushButton *botonPrincipal, QLabel *focoLed, QWidget *parent = nullptr)
        : QDialog(parent), adaptador(adaptadorLocal), socketRobot(socketCompartido), btnMain(botonPrincipal), ledIndicator(focoLed)
    {
        this->setWindowTitle("Control de Dispositivos Bluetooth");
        this->resize(420, 520);
        this->setStyleSheet("background-color: #0d1117; color: white;");

        QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

        // Solución Manual si el escáner falla
        QLabel *lblManual = new QLabel("SOLUCIÓN MANUAL (Si el escáner falla o no encuentra nada):", this);
        lblManual->setStyleSheet("font-weight: bold; font-size: 11px; color: #ff9f1c; margin-top: 5px;");
        layoutPrincipal->addWidget(lblManual);

        QHBoxLayout *layoutManualEntrada = new QHBoxLayout();
        txtMacManual = new QLineEdit(this);
        txtMacManual->setPlaceholderText("Escribe la MAC (Ej: 00:21:13:01:26:A4)");
        txtMacManual->setStyleSheet("color: white; background-color: #161b22; border: 1px solid #30363d; padding: 6px; font-size: 12px;");
        layoutManualEntrada->addWidget(txtMacManual, 1);

        QPushButton *btnConectarManual = new QPushButton("🔗 ENLAZAR", this);
        btnConectarManual->setStyleSheet("color: black; background-color: #ff9f1c; font-weight: bold; padding: 6px; border-radius: 4px; font-size: 11px;");
        layoutManualEntrada->addWidget(btnConectarManual);
        layoutPrincipal->addLayout(layoutManualEntrada);

        // Lista de escaneo
        QLabel *titulo = new QLabel("\nDispositivos Bluetooth Detectados / Vinculados:", this);
        titulo->setStyleSheet("font-weight: bold; font-size: 12px; color: #00f0ff;");
        layoutPrincipal->addWidget(titulo);

        listaDispositivos = new QListWidget(this);
        listaDispositivos->setStyleSheet("color: #c9d1d9; background-color: #161b22; border: 1px solid #30363d; font-size: 13px; padding: 5px;");
        layoutPrincipal->addWidget(listaDispositivos);

        btnEscanear = new QPushButton("🔍 INICIAR ESCANEO", this);
        btnEscanear->setStyleSheet("color: white; background-color: #238636; border: 1px solid #30363d; padding: 10px; font-weight: bold; font-size: 12px; border-radius: 4px;");
        layoutPrincipal->addWidget(btnEscanear);

        QPushButton *btnCancelar = new QPushButton("Cerrar", this);
        btnCancelar->setStyleSheet("color: #c9d1d9; background-color: #21262d; border: 1px solid #30363d; padding: 8px; font-weight: bold; border-radius: 4px;");
        layoutPrincipal->addWidget(btnCancelar);

        agenteDiscovery = new QBluetoothDeviceDiscoveryAgent(this);

        // Cargar vinculados de Windows
        QList<QBluetoothAddress> vinculados = adaptador->connectedDevices();
        for (const auto& direccion : vinculados) {
            listaDispositivos->addItem("Ajustes de Windows (Emparejado)\n" + direccion.toString());
        }

        connect(agenteDiscovery, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, [this](const QBluetoothDeviceInfo &device) {
            QString nombre = device.name().isEmpty() ? "Dispositivo Detectado" : device.name();
            QString mac = device.address().toString();
            QString itemTexto = nombre + "\n" + mac;

            bool existe = false;
            for (int i = 0; i < listaDispositivos->count(); ++i) {
                if (listaDispositivos->item(i)->text().contains(mac)) {
                    existe = true; break;
                }
            }
            if (!existe) listaDispositivos->addItem(itemTexto);
        });

        connect(btnEscanear, &QPushButton::clicked, this, [this]() {
            listaDispositivos->clear();
            btnEscanear->setText("🔄 ESCANEANDO ENTORNO...");
            btnEscanear->setEnabled(false);
            agenteDiscovery->start();
        });

        connect(agenteDiscovery, &QBluetoothDeviceDiscoveryAgent::finished, this, [this]() {
            btnEscanear->setText("🔍 INICIAR ESCANEO");
            btnEscanear->setEnabled(true);
        });

        connect(btnCancelar, &QPushButton::clicked, this, &QDialog::reject);
        connect(listaDispositivos, &QListWidget::itemClicked, this, &DialogoBluetooth::conectarAlSeleccionado);

        connect(btnConectarManual, &QPushButton::clicked, this, [this]() {
            QString macEscrita = txtMacManual->text().trimmed();
            if(!macEscrita.isEmpty()) iniciarEnlaceBluetooth(macEscrita);
        });
    }

    ~DialogoBluetooth() {
        if (agenteDiscovery->isActive()) agenteDiscovery->stop();
    }

private:
    QListWidget *listaDispositivos;
    QLineEdit *txtMacManual;
    QPushButton *btnEscanear;
    QBluetoothLocalDevice *adaptador;
    QBluetoothSocket *socketRobot;
    QPushButton *btnMain;
    QLabel *ledIndicator;
    QBluetoothDeviceDiscoveryAgent *agenteDiscovery;

    void conectarAlSeleccionado(QListWidgetItem *item) {
        QStringList lineas = item->text().split("\n");
        if (lineas.size() < 2 || lineas.at(1).contains(" ")) return;
        iniciarEnlaceBluetooth(lineas.at(1));
    }

    void iniciarEnlaceBluetooth(QString mac) {
        if (agenteDiscovery->isActive()) agenteDiscovery->stop();
        QBluetoothUuid uuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));

        QObject::disconnect(socketRobot, &QBluetoothSocket::connected, nullptr, nullptr);
        QObject::disconnect(socketRobot, &QBluetoothSocket::errorOccurred, nullptr, nullptr);

        socketRobot->connectToService(QBluetoothAddress(mac), uuid);

        connect(socketRobot, &QBluetoothSocket::connected, this, [this]() {
            btnMain->setText("¡CONECTADO!");
            btnMain->setStyleSheet("color: white; background-color: #00f0ff; font-weight: bold; font-size: 11px;");
            ledIndicator->setStyleSheet("background-color: #39FF14; border: 2px solid #ffffff; border-radius: 10px; min-width: 20px; min-height: 20px;");
            QMessageBox::information(this, "Éxito", "¡Enlace Bluetooth establecido!");
            this->accept();
        });

        connect(socketRobot, &QBluetoothSocket::errorOccurred, this, [this](QBluetoothSocket::SocketError) {
            QMessageBox::critical(this, "Error", "No se pudo conectar. Verifica que el dispositivo esté encendido.");
            btnEscanear->setText("🔍 INICIAR ESCANEO");
            btnEscanear->setEnabled(true);
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
        contenido->setText("• Forward = F\n\n• Backward = B\n\n• Left = L\n\n• Right = R\n\n• Stop = S");
        contenido->setStyleSheet("font-size: 13px; color: #c9d1d9; background-color: #161b22; border: 1px solid #30363d; padding: 15px; border-radius: 5px;");
        layout->addWidget(contenido);

        QPushButton *btnEntendido = new QPushButton("Entendido", this);
        btnEntendido->setStyleSheet("color: white; background-color: #21262d; padding: 8px; font-weight: bold;");
        layout->addWidget(btnEntendido);
        connect(btnEntendido, &QPushButton::clicked, this, &QDialog::accept);
    }
};
// ============================================================================
// INTERFAZ PRINCIPAL: MainWindow (Constructor completo sin cortes de llaves)
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

    txtIpVideo = new QLineEdit(this);
    txtIpVideo->setPlaceholderText("IP:Puerto (Ej: 192.168.1.27:8080)");
    txtIpVideo->setStyleSheet("color: white; background-color: #161b22; border: 1px solid #30363d; padding: 8px;");

    btnConectarVideo = new QPushButton("APLICAR IP", this);
    btnConectarVideo->setStyleSheet("color: white; background-color: #21262d; border: 1px solid #30363d; padding: 8px;");

    btnBluetooth = new QPushButton("BLUETOOTH", this);
    btnBluetooth->setStyleSheet("color: white; background-color: #238636; border: 1px solid #30363d; padding: 8px;");

    lblFocoLed = new QLabel(this);
    lblFocoLed->setStyleSheet("background-color: #FF3131; border: 2px solid #ffffff; border-radius: 10px; min-width: 20px; min-height: 20px; max-width: 20px; max-height: 20px;");

    QPushButton *btnSettings = new QPushButton("⚙ SETTINGS", this);
    btnSettings->setStyleSheet("color: #00f0ff; background-color: #161b22; border: 1px solid #00f0ff; padding: 8px; border-radius: 4px;");

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

    QVBoxLayout *layoutIzquierdo = new QVBoxLayout();
    layoutIzquierdo->addStretch();
    layoutIzquierdo->addWidget(btnIzquierda, 0, Qt::AlignCenter);
    layoutIzquierdo->addSpacing(20);
    layoutIzquierdo->addWidget(btnDerecha, 0, Qt::AlignCenter);
    layoutIzquierdo->addStretch();

    QVBoxLayout *layoutMedio = new QVBoxLayout();
    layoutMedio->addWidget(lblMonitorVideo, 0, Qt::AlignCenter);

    QVBoxLayout *layoutDerecho = new QVBoxLayout();
    layoutDerecho->addStretch();
    layoutDerecho->addWidget(btnArriba, 0, Qt::AlignCenter);
    layoutDerecho->addSpacing(20);
    layoutDerecho->addWidget(btnAbajo, 0, Qt::AlignCenter);
    layoutDerecho->addStretch();

    layoutControlesAbajo->addLayout(layoutIzquierdo, 1);
    layoutControlesAbajo->addLayout(layoutMedio, 4);
    layoutControlesAbajo->addLayout(layoutDerecho, 1);

    layoutEstructuraVertical->addLayout(layoutControlesAbajo);

    socketBluetooth = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    managerRedVideo = new QNetworkAccessManager(this);
    relojVideoTiempoReal = new QTimer(this);

    connect(btnBluetooth, &QPushButton::clicked, this, [this]() {
        QBluetoothLocalDevice adaptadorLocal(this);
        DialogoBluetooth ventanaEmergente(&adaptadorLocal, socketBluetooth, btnBluetooth, lblFocoLed, this);
        ventanaEmergente.exec();
    });

    connect(btnSettings, &QPushButton::clicked, this, [this]() { DialogoSettings v(this); v.exec(); });
    connect(btnConectarVideo, &QPushButton::clicked, this, &MainWindow::aplicarNuevaIpVideo);
    connect(relojVideoTiempoReal, &QTimer::timeout, this, &MainWindow::solicitarSiguienteFotograma);
    connect(managerRedVideo, &QNetworkAccessManager::finished, this, &MainWindow::cargarFotogramaEnPantalla);

    connect(btnArriba, &QPushButton::pressed, this, [this]() { btnArriba->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverAdelante(); });
    connect(btnArriba, &QPushButton::released, this, [this]() { btnArriba->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); });
    connect(btnAbajo, &QPushButton::pressed, this, [this]() { btnAbajo->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverAtras(); });
    connect(btnAbajo, &QPushButton::released, this, [this]() { btnAbajo->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); });
    connect(btnIzquierda, &QPushButton::pressed, this, [this]() { btnIzquierda->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverIzquierda(); });
    connect(btnIzquierda, &QPushButton::released, this, [this]() { btnIzquierda->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); });
    connect(btnDerecha, &QPushButton::pressed, this, [this]() { btnDerecha->setStyleSheet(ESTILO_BOTON_PRESIONADO); moverDerecha(); });
    connect(btnDerecha, &QPushButton::released, this, [this]() { btnDerecha->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); });
}
MainWindow::~MainWindow() {
    if (socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->close();
}

void MainWindow::aplicarNuevaIpVideo() {
    QString urlTexto = txtIpVideo->text().trimmed();
    if(urlTexto.isEmpty()) return;
    urlTexto.remove("http://").remove("https://").remove("/video").remove("/shot.jpg");
    if (urlTexto.endsWith("/")) urlTexto = urlTexto.chopped(1);
    urlFormateadaVideo = "http://" + urlTexto + "/shot.jpg";
    lblMonitorVideo->setText("INICIANDO FLUJO EN TIEMPO REAL...");
    relojVideoTiempoReal->start(40);
}

void MainWindow::solicitarSiguienteFotograma() {
    if (urlFormateadaVideo.isEmpty()) return;
    managerRedVideo->get(QNetworkRequest(QUrl(urlFormateadaVideo)));
}

void MainWindow::cargarFotogramaEnPantalla(QNetworkReply *reply) {
    if (!reply) return;
    if (reply->error() == QNetworkReply::NoError) {
        QImage fotograma;
        if (fotograma.loadFromData(reply->readAll(), "JPEG")) {
            lblMonitorVideo->setPixmap(QPixmap::fromImage(fotograma).scaled(lblMonitorVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        lblMonitorVideo->setText("ERROR DE ENLACE CON IP WEBCAM");
        relojVideoTiempoReal->stop();
    }
    reply->deleteLater();
}

void MainWindow::moverAdelante() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("F"); }
void MainWindow::moverAtras() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("B"); }
void MainWindow::moverIzquierda() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("L"); }
void MainWindow::moverDerecha() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("R"); }
void MainWindow::detenerRobot() { if(socketBluetooth && socketBluetooth->isOpen()) socketBluetooth->write("S"); }
