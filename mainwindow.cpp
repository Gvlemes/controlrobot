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
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

// Estilos corregidos: Eliminamos max-width y max-height para evitar bloqueos en Android
const QString ESTILO_BOTON_NORMAL = 
    "color: #00f0ff; background-color: #21262d; border: 2px solid #00f0ff; "
    "font-size: 22px; padding: 10px; min-width: 65px; min-height: 60px; border-radius: 8px;"; 

const QString ESTILO_BOTON_PRESIONADO = 
    "color: #ffffff; background-color: #005f73; border: 3px solid #00f0ff; "
    "font-size: 22px; padding: 10px; min-width: 65px; min-height: 60px; border-radius: 8px; font-weight: bold;"; 

// ============================================================================ 
// SUB-VENTANA EMERGENTE: Panel Bluetooth Seguro con Conexión por MAC 
// ============================================================================ 
class DialogoBluetooth : public QDialog { 
public: 
    DialogoBluetooth(QBluetoothLocalDevice *adaptadorLocal, QBluetoothSocket *socketCompartido, 
                     QPushButton *botonPrincipal, QLabel *focoLed, QWidget *parent = nullptr) 
        : QDialog(parent), adaptador(adaptadorLocal), socketRobot(socketCompartido), 
          btnMain(botonPrincipal), ledIndicator(focoLed) { 
        
        this->setWindowTitle("Hardware Bluetooth");
        this->resize(340, 400); 
        this->setStyleSheet("background-color: #0d1117; color: white;"); 
        
        QVBoxLayout *layoutPrincipal = new QVBoxLayout(this); 
        layoutPrincipal->setContentsMargins(10, 10, 10, 10); 
        
        QLabel *lblManual = new QLabel("CONEXIÓN DIRECTA POR MAC:", this); 
        lblManual->setStyleSheet("font-weight: bold; font-size: 11px; color: #ff9f1c;"); 
        layoutPrincipal->addWidget(lblManual); 
        
        QHBoxLayout *layoutManualEntrada = new QHBoxLayout(); 
        txtMacManual = new QLineEdit(this); 
        txtMacManual->setPlaceholderText("Ej: 00:23:10:01:14:4D"); 
        txtMacManual->setStyleSheet("color: white; background-color: #161b22; border: 1px solid #30363d; padding: 6px; font-size: 12px;"); 
        layoutManualEntrada->addWidget(txtMacManual, 1); 
        
        QPushButton *btnConectarManual = new QPushButton("🔗 ENLAZAR", this); 
        btnConectarManual->setStyleSheet("color: black; background-color: #ff9f1c; font-weight: bold; padding: 6px; font-size: 11px; border-radius: 4px;"); 
        layoutManualEntrada->addWidget(btnConectarManual); 
        layoutPrincipal->addLayout(layoutManualEntrada); 
        
        QLabel *titulo = new QLabel("Dispositivos Detectados:", this); 
        titulo->setStyleSheet("font-weight: bold; font-size: 11px; color: #00f0ff; margin-top: 5px;");
        layoutPrincipal->addWidget(titulo); 
        listaDispositivos = new QListWidget(this); 
        listaDispositivos->setStyleSheet("color: #c9d1d9; background-color: #161b22; border: 1px solid #30363d; font-size: 12px;"); 
        layoutPrincipal->addWidget(listaDispositivos); 
        
        btnEscanear = new QPushButton("🔍 INICIAR ESCANEO", this); 
        btnEscanear->setStyleSheet("color: white; background-color: #238636; border: 1px solid #30363d; min-height: 35px; font-weight: bold; font-size: 11px; border-radius: 4px;"); 
        layoutPrincipal->addWidget(btnEscanear); 
        
        QPushButton *btnCancelar = new QPushButton("❌ CERRAR PANEL", this); 
        btnCancelar->setStyleSheet("color: #c9d1d9; background-color: #21262d; border: 1px solid #30363d; min-height: 35px; font-weight: bold; font-size: 11px; border-radius: 4px;"); 
        layoutPrincipal->addWidget(btnCancelar); 
        
        agenteDiscovery = new QBluetoothDeviceDiscoveryAgent(this); 
        QList<QBluetoothAddress> vinculados = adaptador->connectedDevices(); 
        for (const auto& direccion : vinculados) { 
            listaDispositivos->addItem("Dispositivo Vinculado\n" + direccion.toString()); 
        } 
        
        connect(agenteDiscovery, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, 
        [this](const QBluetoothDeviceInfo &device) { 
            QString nombre = device.name().isEmpty() ? "Módulo Bluetooth" : device.name(); 
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
            btnEscanear->setText("🔄 BUSCANDO SEÑALES..."); 
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
            QString macEscrita = txtMacManual->text().trimmed().toUpper(); 
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
            ledIndicator->setStyleSheet("background-color: #39FF14; border: 2px solid #ffffff; border-radius: 9px; min-width: 18px; min-height: 18px; max-width: 18px; max-height: 18px;"); 
            QMessageBox::information(this, "Éxito", "¡Enlace establecido!"); 
            this->accept(); 
        }); 

        connect(socketRobot, &QBluetoothSocket::errorOccurred, this, [this](QBluetoothSocket::SocketError) { 
            QMessageBox::critical(this, "Error", "Fallo de conexión serial física."); 
            btnEscanear->setText("🔍 INICIAR ESCANEO"); 
            btnEscanear->setEnabled(true); 
        }); 
    } 
}; 

// ============================================================================ 
// SUB-VENTANA EMERGENTE: Ajustes del Sistema
// ============================================================================ 
class DialogoSettings : public QDialog { 
public: 
    DialogoSettings(QWidget *parent = nullptr) : QDialog(parent) { 
        this->setWindowTitle("Ajustes del Sistema"); 
        this->resize(300, 240); 
        this->setStyleSheet("background-color: #0d1117; color: white;"); 
        QVBoxLayout *layout = new QVBoxLayout(this); 
        
        QLabel *titulo = new QLabel("⚙️ MAPEO DE COMANDOS SERIALES", this); 
        titulo->setStyleSheet("font-weight: bold; font-size: 12px; color: #00f0ff;"); 
        layout->addWidget(titulo); 

        QLabel *contenido = new QLabel(this); 
        contenido->setText("• Adelante = F\n• Atrás = B\n• Izquierda = L\n• Derecha = R\n• Parar = S"); 
        contenido->setStyleSheet("font-size: 12px; color: #c9d1d9; background-color: #161b22; border: 1px solid #30363d; padding: 10px; border-radius: 5px;"); 
        layout->addWidget(contenido); 

        QPushButton *btnEntendido = new QPushButton("Entendido", this); 
        btnEntendido->setStyleSheet("color: white; background-color: #21262d; padding: 6px; font-weight: bold;"); 
        layout->addWidget(btnEntendido); 
        
        connect(btnEntendido, &QPushButton::clicked, this, &QDialog::accept); 
    } 
};
// ============================================================================ 
// VENTANA PRINCIPAL: Inicialización y Controles Responsivos Híbridos
// ============================================================================ 
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) { 
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

    btnArriba->setAutoRepeat(false); 
    btnAbajo->setAutoRepeat(false); 
    btnIzquierda->setAutoRepeat(false); 
    btnDerecha->setAutoRepeat(false); 
    
    lblMonitorVideo = new QLabel("MONITOR DE VIDEO (STANDBY)", this); 
    lblMonitorVideo->setAlignment(Qt::AlignCenter);
    lblMonitorVideo->setStyleSheet("background-color: black; color: #00f0ff; border: 2px solid #00f0ff; font-weight: bold; font-size: 13px;"); 
    
    // Configuración Responsiva: Forzamos al video a respetar la altura de los botones en móvil
    lblMonitorVideo->setMinimumSize(200, 120); 
    lblMonitorVideo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored); 
    
    txtIpVideo = new QLineEdit(this); 
    txtIpVideo->setPlaceholderText("IP de tu IP Webcam (Ej: 192.168.1.50:8080)"); 
    txtIpVideo->setStyleSheet("color: white; background-color: #161b22; border: 1px solid #30363d; padding: 6px; font-size: 11px;"); 
    
    btnConectarVideo = new QPushButton("APLICAR IP", this); 
    btnConectarVideo->setStyleSheet("color: white; background-color: #21262d; border: 1px solid #30363d; padding: 6px; font-weight: bold; font-size: 11px;"); 
    
    btnBluetooth = new QPushButton("BLUETOOTH", this); 
    btnBluetooth->setStyleSheet("color: white; background-color: #238636; border: 1px solid #30363d; padding: 6px; font-weight: bold; font-size: 11px;"); 
    
    lblFocoLed = new QLabel(this); 
    lblFocoLed->setStyleSheet("background-color: #FF3131; border: 2px solid #ffffff; border-radius: 9px; min-width: 18px; min-height: 18px; max-width: 18px; max-height: 18px;"); 

    QPushButton *btnSettings = new QPushButton("⚙️ SETTINGS", this); 
    btnSettings->setStyleSheet("color: #00f0ff; background-color: #161b22; border: 1px solid #00f0ff; padding: 6px; border-radius: 4px; font-weight: bold; font-size: 11px;"); 
    
    QVBoxLayout *layoutEstructuraVertical = new QVBoxLayout(central); 
    layoutEstructuraVertical->setContentsMargins(8, 8, 8, 8); 
    layoutEstructuraVertical->setSpacing(6); 
    
    QHBoxLayout *layoutSuperiorEsparcido = new QHBoxLayout(); 
    layoutSuperiorEsparcido->addWidget(btnBluetooth); 
    layoutSuperiorEsparcido->addWidget(lblFocoLed);
    layoutSuperiorEsparcido->addSpacing(8); 
    layoutSuperiorEsparcido->addWidget(txtIpVideo, 1); 
    layoutSuperiorEsparcido->addWidget(btnConectarVideo); 
    layoutSuperiorEsparcido->addSpacing(8); 
    layoutSuperiorEsparcido->addWidget(btnSettings); 
    
    layoutEstructuraVertical->addLayout(layoutSuperiorEsparcido, 1); 
    
    QHBoxLayout *layoutControlesAbajo = new QHBoxLayout(); 
    
    // 1. Columna Izquierda: Adelante y Atrás
    QVBoxLayout *layoutVerticalMover = new QVBoxLayout(); 
    layoutVerticalMover->addStretch(); 
    layoutVerticalMover->addWidget(btnArriba, 0, Qt::AlignCenter); 
    layoutVerticalMover->addSpacing(15); 
    layoutVerticalMover->addWidget(btnAbajo, 0, Qt::AlignCenter); 
    layoutVerticalMover->addStretch(); 
    
    // 2. Columna Central Correjida: Video arriba y Flechas fijas Abajo
    QVBoxLayout *layoutCentroMultimedia = new QVBoxLayout(); 
    layoutCentroMultimedia->addWidget(lblMonitorVideo, 3); // Le asignamos mayor peso proporcional al video
    layoutCentroMultimedia->addSpacing(8); 
    
    QHBoxLayout *layoutFilaGiros = new QHBoxLayout(); 
    layoutFilaGiros->addStretch(1); // Empuja dinámicamente desde la izquierda
    layoutFilaGiros->addWidget(btnIzquierda); 
    layoutFilaGiros->addSpacing(20); // Espacio seguro intermedio para pantallas angostas
    layoutFilaGiros->addWidget(btnDerecha); 
    layoutFilaGiros->addStretch(1); // Empuja dinámicamente desde la derecha
    
    layoutCentroMultimedia->addLayout(layoutFilaGiros, 1); 
    
    layoutControlesAbajo->addLayout(layoutVerticalMover, 1); 
    layoutControlesAbajo->addLayout(layoutCentroMultimedia, 2); 
    layoutEstructuraVertical->addLayout(layoutControlesAbajo, 6); 
    
    socketBluetooth = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this); 
    managerRedVideo = new QNetworkAccessManager(this); 
    
    relojVideoTiempoReal = new QTimer(this); 
    comandoActual = 'S'; 
    relojRepetidorBluetooth = new QTimer(this); 
    
    connect(btnBluetooth, &QPushButton::clicked, this, [this]() { 
        QBluetoothLocalDevice adaptadorLocal(this);
        DialogoBluetooth ventanaEmergente(&adaptadorLocal, socketBluetooth, btnBluetooth, lblFocoLed, this); 
        ventanaEmergente.exec(); 
    }); 
    
    connect(btnSettings, &QPushButton::clicked, this, [this]() { DialogoSettings v(this); v.exec(); }); 
    connect(btnConectarVideo, &QPushButton::clicked, this, &MainWindow::aplicarNuevaIpVideo); 
    
    connect(relojVideoTiempoReal, &QTimer::timeout, this, &MainWindow::solicitarSiguienteFotograma); 
    connect(managerRedVideo, &QNetworkAccessManager::finished, this, &MainWindow::cargarFotogramaEnPantalla); 
    connect(relojRepetidorBluetooth, &QTimer::timeout, this, &MainWindow::transmitirComandoActivo); 
    
    // Conexiones de Eventos Táctiles (Presionado y Liberado)
    connect(btnArriba, &QPushButton::pressed, this, [this]() { btnArriba->setStyleSheet(ESTILO_BOTON_PRESIONADO); comandoActual = 'F'; relojRepetidorBluetooth->start(50); }); 
    connect(btnArriba, &QPushButton::released, this, [this]() { btnArriba->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); }); 
    
    connect(btnAbajo, &QPushButton::pressed, this, [this]() { btnAbajo->setStyleSheet(ESTILO_BOTON_PRESIONADO); comandoActual = 'B'; relojRepetidorBluetooth->start(50); }); 
    connect(btnAbajo, &QPushButton::released, this, [this]() { btnAbajo->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); }); 
    
    connect(btnIzquierda, &QPushButton::pressed, this, [this]() { btnIzquierda->setStyleSheet(ESTILO_BOTON_PRESIONADO); comandoActual = 'R'; relojRepetidorBluetooth->start(50); }); 
    connect(btnIzquierda, &QPushButton::released, this, [this]() { btnIzquierda->setStyleSheet(ESTILO_BOTON_NORMAL); detenerRobot(); }); 
    
    connect(btnDerecha, &QPushButton::pressed, this, [this]() { btnDerecha->setStyleSheet(ESTILO_BOTON_PRESIONADO); comandoActual = 'L'; relojRepetidorBluetooth->start(50); }); 
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
    lblMonitorVideo->setText("CONECTANDO STREAMING REGULADO..."); 
    relojVideoTiempoReal->start(100); 
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
        QByteArray bufferImagen = reply->readAll();
        QImage fotograma; 
        if (!bufferImagen.isEmpty() && fotograma.loadFromData(bufferImagen, "JPEG")) { 
            lblMonitorVideo->setPixmap(QPixmap::fromImage(fotograma).scaled(lblMonitorVideo->size(), Qt::KeepAspectRatio, Qt::FastTransformation)); 
        } 
    } else { 
        lblMonitorVideo->setText("ERROR DE RESPUESTA\nIP Webcam saturada."); 
    } 
    reply->deleteLater(); 
} 

void MainWindow::transmitirComandoActivo() { 
    if (socketBluetooth && socketBluetooth->isOpen() && comandoActual != 'S') { 
        QByteArray datos; 
        datos.append(comandoActual); 
        socketBluetooth->write(datos); 
    } 
} 

void MainWindow::moverAdelante() { } 
void MainWindow::moverAtras() { } 
void MainWindow::moverIzquierda() { } 
void MainWindow::moverDerecha() { } 

void MainWindow::detenerRobot() { 
    relojRepetidorBluetooth->stop(); 
    comandoActual = 'S'; 
    if (socketBluetooth && socketBluetooth->isOpen()) { 
        socketBluetooth->write("S"); 
    } 
}
