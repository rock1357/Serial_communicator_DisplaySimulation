#include "PRDisplay.h"
#include "./ui_PRDisplay.h"
#include "qthread.h"
#include <QToolBar>
#include <QActionGroup>
#include <QSettings>
#include <QTimer>
#include <QMap>

//QMap<int,int> mapTable=
//{
//    {1,38},
//    {2,146},
//}

int lookUpTable[]={-1,38,146,148,150,152,154,156,158};


PRDisplay::PRDisplay(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PRDisplay)
{

    //Create front end
    initUI();
    //Create back end
    initStateMachine();

    //esempi di insert nella qmap, con le due possibilità (o insert, o parentesi quadre)
    //mapTable.insert(1,38);
    //mapTable.insert(2,146);
    //mapTable[1]=38;
    //mapTable[2]=146;

    // regexp: optional '-' followed by between 1 and 3 digits
    QRegularExpression rx("[-HELPhelp\\d\\ .]*");
    QValidator *validator = new QRegularExpressionValidator(rx, this);

    ui->leValues->setValidator(validator);
    ui->leValues->setToolTip("Accepted characters: [A to F]-[a to f]-[0 to 9]");
    ui->sbRegData->setMaximum(std::numeric_limits<int>::max());

    ui->twPM2->setCurrentIndex(0); // Imposta il tab 1 all'avvio

    ui->cbRegisters->addItem("Registers:",0);
    ui->cbRegisters->addItem("38 - SOFTWARE VERSION: ", QVariant(38));
    ui->cbRegisters->addItem("146 - WRITE UNIT PRICE L", QVariant(146));
    ui->cbRegisters->addItem("148 - PRICE L            ", QVariant(148));
    ui->cbRegisters->addItem("150 - PRICE H            ", QVariant(150));
    ui->cbRegisters->addItem("152 - TOTAL PRICE L      ", QVariant(152));
    ui->cbRegisters->addItem("156 - TOTAL AMOUNT L     ", QVariant(154));
    ui->cbRegisters->addItem("156 - TOTAL AMOUNT H     ", QVariant(156));

    emit ui->twPM2->currentChanged(0);
    emit SetRegAdrs(lookUpTable[0]);
}

PRDisplay::~PRDisplay()
{
    delete ui;
}

void PRDisplay::paintEvent(QPaintEvent *event)
{
    //This way the UI stays at the minimum size, but it's still capable of resizing
    //to allow proper display of widgets
    this->resize(this->sizeHint());

}

void PRDisplay::initUI()
{
    //Instantiate About before UI
    m_About=new About(this);

    //Instantiate UI after About (so custom widgets can use About properties)
    ui->setupUi(this);

    //Connect Action for About
    connect(ui->actionAbout,&QAction::triggered,m_About,&About::show);

    //Create tool bar & link it's actions
    initTB();

    //Load last settings from .ini default file
    loadSettings();

    //set interface
    Qt::WindowFlags flags=Qt::WindowType::Window;
    flags |= Qt::WindowTitleHint;
    flags |= Qt::WindowMinimizeButtonHint;
    flags |= Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    setWindowTitle(m_About->info().swName);

}

void PRDisplay::initTB()
{
    //Create toolbar instance and add it to the main window
    m_toolBar = new QToolBar(this);
    addToolBar(m_toolBar);

    //Create connection widget and add it to toolbar
    m_CW=new ConnectionWidget("Master",this);
    m_CW->setReLinkTimeout(3000);
    m_CW->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    m_toolBar->addWidget(m_CW);

}

void PRDisplay::initStateMachine()
{
    //Build a new trade in which put the StateMachine
    InitSMThread();

    COMPort::Param_t par("");    
    m_CW->setPortParam(par);

    //function that make all the connections referred to Tab1 methods which are related to display management
    Tab1Connections();
    //function that make all the connections referred to Tab2 methods which are related to PM2 management
    Tab2Connections();

}

void PRDisplay::InitSMThread(){

    //Create thread manager for state machine
    m_SMThread_M=new QThread(this);

    //Create state machine
    m_SM_M=new StateMachine(m_CW);

    //move the StateMachine instance into a new thread
    m_SM_M->moveToThread(m_SMThread_M);

    //Clean StateMachineMbM object when ui destroyes m_SMThread_M
    connect(m_SMThread_M,&QThread::finished,m_SM_M,&StateMachine::deleteLater,Qt::DirectConnection);

    //Start StateMachineMbM thread
    m_SMThread_M->start(QThread::HighestPriority);
}

void PRDisplay::Tab1Connections(){    

    //These connects are for the counter activation.
    connect(this,&PRDisplay::SetCounterAction,m_SM_M,&StateMachine::OnSetCounterActivation,Qt::QueuedConnection);

    connect(ui->pbCounter,&QPushButton::clicked,this,[&](){
        emit SetCounterAction(ui->cbDisplays->currentIndex());
    });


    //These next two connects serves for displaing numbers on the screen
    connect(this,&PRDisplay::SetLCDnumbers,m_SM_M,&StateMachine::OnSetLCDnumbers,Qt::QueuedConnection);

    connect(ui->pbSetDispValues,&QPushButton::clicked,this,[&](){
        QString text = ui->leValues->text();
        if (text.length() != 6) {
            ui->leThrowExcp->setText("WARNING: the message does not have the correct length");
        }
        emit SetLCDnumbers((FWUtils::LCD)ui->cbLCDPos->currentIndex(),ui->cbDisplays->currentIndex(),text);
    });

    //Connection to show if the bytes relative to the expected/correct response were recieved
    connect(m_SM_M,&StateMachine::ShowStatus,this,&PRDisplay::OnShowStatus,Qt::QueuedConnection);

    //Connection to turn On/Off light on display
    connect(this,&PRDisplay::LightOnClicked,m_SM_M,&::StateMachine::OnLightOnOffClicked,Qt::QueuedConnection);
    connect(this,&PRDisplay::LightOffClicked,m_SM_M,&::StateMachine::OnLightOnOffClicked,Qt::QueuedConnection);
    connect(ui->pbLightOn,&QPushButton::clicked,this,[&](){
        emit LightOnClicked(ui->cbDisplays->currentIndex());
    });
    connect(ui->pbLightOff,&QPushButton::clicked,this,[&](){
        emit LightOffClicked(ui->cbDisplays->currentIndex());
    });

}

void PRDisplay::Tab2Connections(){

    // Connection to save the register address in a variable of the state machine.
    // connect(ui->cbRegisters,&QComboBox::currentIndexChanged,this,[&](int indexReg){

    //     //if (mapTable.contains(indexReg)) {
    //     //    emit SetRegAdrs(mapTable[indexReg]);
    //     //}&
    //     emit SetRegAdrs(lookUpTable[indexReg]);

    // });


    connect(ui->pbSetValuesToReg, &QPushButton::clicked, this, [&](){
        // Convert the decimal string from the spinbox to a QByteArray containing its hexadecimal representation
        QString decimalString = ui->sbRegData->text();
        int decimalValue = decimalString.toInt(nullptr, 10);
        QByteArray hexData = QByteArray::fromHex(QByteArray::number(decimalValue, 16));

        // Determine the length of hexData to understand which data type to use
        m_Datalength =hexData.length();

        if (m_Datalength == 1) {
            quint8 value = static_cast<quint8>(hexData[0]);
            emit SetRegButtonClicked(ui->sbPMAdr->value(),lookUpTable[ui->cbRegisters->currentIndex()],value);

        } else if (m_Datalength == 2) {
            quint16 value = 0;
            for (int i = 0; i < m_Datalength; ++i) {
                value |= (static_cast<quint16>(static_cast<quint8>(hexData[i])) << (8 * ((m_Datalength - 1) - i)));
            }
            emit SetRegButtonClicked(ui->sbPMAdr->value(),lookUpTable[ui->cbRegisters->currentIndex()],value);

        } else if (m_Datalength <= 4) {
            quint32 value = 0;
            for (int i = 0; i < m_Datalength; ++i) {
                value |= (static_cast<quint32>(static_cast<quint8>(hexData[i])) << (8 * ((m_Datalength - 1) - i)));
            }
            emit SetRegButtonClicked(ui->sbPMAdr->value(),lookUpTable[ui->cbRegisters->currentIndex()],value);

        } else {
            emit SetRegButtonClicked(ui->sbPMAdr->value(),lookUpTable[ui->cbRegisters->currentIndex()],hexData);
        }
    });


    //These 4 connects are made in order to overload the signal related to the slots
    connect(this,QOverload<uint8_t,uint16_t,quint8>::of(&PRDisplay::SetRegButtonClicked),m_SM_M,QOverload<uint8_t ,uint16_t ,quint8>::of(&StateMachine::OnSetRegButtonClicked));
    connect(this,QOverload<uint8_t,uint16_t,quint16>::of(&PRDisplay::SetRegButtonClicked),m_SM_M,QOverload<uint8_t ,uint16_t ,quint16>::of(&StateMachine::OnSetRegButtonClicked));
    connect(this,QOverload<uint8_t,uint16_t,quint32>::of(&PRDisplay::SetRegButtonClicked),m_SM_M,QOverload<uint8_t ,uint16_t ,quint32>::of(&StateMachine::OnSetRegButtonClicked));
    connect(this,QOverload<uint8_t,uint16_t,QByteArray>::of(&PRDisplay::SetRegButtonClicked),m_SM_M,QOverload<uint8_t ,uint16_t ,QByteArray>::of(&StateMachine::OnSetRegButtonClicked));

    // Connect that serves to obtain the infos stored in the Register at the selected address
    //connect(ui->pbGetValuesFromReg,&QPushButton::clicked,m_SM_M,&::StateMachine::OnGetRegButtonClicked,Qt::QueuedConnection);
    /*  Another possible version to send datalength to read register:*/
    connect(this,&PRDisplay::GetRegButtonClicked,m_SM_M,&StateMachine::OnGetRegButtonClicked,Qt::QueuedConnection);
    connect(ui->pbGetValuesFromReg,&QPushButton::clicked,this,[&](){
        emit GetRegButtonClicked(ui->sbPMAdr->value(),lookUpTable[ui->cbRegisters->currentIndex()]);});

    //connect to display infos about the register
    connect(m_SM_M,&StateMachine::ShowRegValues,this,&PRDisplay::OnShowRegValues,Qt::QueuedConnection);


    //Connection to show the recieved fina exception after having processed the buffer
    connect(m_SM_M,&StateMachine::ShowException,this,&PRDisplay::OnShowEXCP,Qt::QueuedConnection);

    // I want to make a signal (from the class tabwidgets) through which i can disable the pollim timer of the state machine
    connect(ui->twPM2, &QTabWidget::currentChanged, m_SM_M, &StateMachine::onTabChanged);

}

void PRDisplay::saveSettings()
{
    QSettings SET INIT_FILE;
    //data about image printing procedure parameters
    SET.beginGroup("PAR_PRG");

    SET.endGroup();
}

void PRDisplay::loadSettings()
{
    QSettings SET INIT_FILE;
    SET.beginGroup("PAR_PRG");
    QStringList keys=SET.allKeys();


    if(keys.isEmpty()) {
        SET.endGroup();
        //No settings found: init them
        m_prgTitle=m_About->info().swName;
        saveSettings();
    } else {
        //Settings found: load them
        m_prgTitle=SET.value("FWMaster").toString();

        SET.endGroup();
    }
}

void PRDisplay::OnShowStatus( QString Status )
{

    ui->teUserInfos->clear();

    ui->teUserInfos->append(Status);

}

void PRDisplay::OnShowEXCP( QString Status )
{

    ui->leThrowExcp->clear();

    ui->leThrowExcp->setText(Status);

}

void PRDisplay::OnShowRegValues(QString str, QByteArray values) {

    ui->teRegInfos->clear();
    // Remove non-hex characters from the QByteArray
    if(values.length()!=0 || values!=nullptr){
        QString hexString = QString(values.toHex());
        int decimalValue = hexString.toInt(nullptr, 16);
        QString decimalString = QString::number(decimalValue);
        ui->teRegInfos->append(str);
        ui->teRegInfos->append(decimalString + "(0x" + hexString.toUpper() + ")");
    } else {
        ui->teRegInfos->append(str);
    }

}




