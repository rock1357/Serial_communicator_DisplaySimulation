#ifndef FWMASTER_H
#define FWMASTER_H

#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QFile>
#include <ConnectionWidget.h>
#include "FWUtils.h"

class FWMaster : public QObject
{
    Q_OBJECT
public:
    explicit FWMaster(QObject *parent = nullptr,ConnectionWidget *cw=nullptr);
    ~FWMaster();

    //Method to dynamically get/set guaranteed delay (ms) between consecutive messages
    uint delay(){return m_par.delay;};
    void setDelay(const uint &delay){m_par.delay = delay;};

    //Method to dynamically get/set maximum number of attempt of sending a message before retunring an exception
    uint maxTry(){return m_par.maxTry;};
    void setMaxTry(const uint &maxTry){m_par.maxTry = maxTry;};

    //Methods to dynamically get/set slaveID with thich MBMaster will communicate
    uint8_t slaveID(){return m_par.slaveID;};

    void setSlaveID(const uint8_t &slaveID){

            m_par.slaveID = slaveID;
    };

    //Method to dynamically get/set log status
    bool log(){return m_par.doLog;};
    void setLog(const bool &doLog){m_par.doLog = doLog; manageLog();};

    // this helper function is used to set the specific LCD address. This helps avoid code duplication and makes the code more maintainable.
    FWUtils::EXCP SetLCD(FWUtils::LCD lcd, QString &text);

    //Method to turn On the light
    FWUtils::EXCP SetBackLight(bool OnOff);

    //To make the writeFWReg private but with the possibility to be called from the function in StateMachine, which simply send a quint8, i need a function caller:
    FWUtils::EXCP SetReg(const quint16 RegAdr,const quint8 SetData);
    FWUtils::EXCP SetReg(const quint16 RegAdr,const quint16 SetData);
    FWUtils::EXCP SetReg(const quint16 RegAdr,const quint32 SetData);
    FWUtils::EXCP SetReg(const quint16 RegAdr,const QByteArray SetData);

    //same thing for the reading function (of quint8) from registers
    FWUtils::EXCP GetReg(const quint16 RegAdr, quint8 &Data);
    FWUtils::EXCP GetReg(const quint16 RegAdr, quint16 &Data);
    FWUtils::EXCP GetReg(const quint16 RegAdr, quint32 &Data);
    FWUtils::EXCP GetReg(const quint16 RegAdr, QByteArray &Data);


    //method to check the display
    FWUtils::EXCP CheckDisp(bool check, bool &DStatus, bool &CStatus);  

    //Method to perform a FW transaction (send txBuff & wait for rxBuff of expected rxLen, retry if no response is issued)
    //WARNING: since this method doesNOT parse the response, it's unable to update the ConnectionWidget led
    bool doTransaction(QByteArray &txBuff, QByteArray *rxBuff);

signals:
    //Signal dedicated to update front end when connection status changes
    void linkFeedback(const bool &linkStat);

    //Signal dedicated to manipulate led of Connection Widget to signal connection status & transaction results
    void setLed(const QString &ledName);

    void PortStatus(QString Status, bool DStat, bool CStat);


public slots:
    //Slot dedicated to react to setLink signal from Connection Widget
    void onSetLink(const COMPort::Param_t &par);

    FWUtils::EXCP SendCmnd(QByteArray &buffTx, QByteArray *buffRx=nullptr);   

private:
    //Open (overwrite) or close log file based on current connection status
    void manageLog();

    FWUtils::EXCP PayLoadCheck(const QByteArray &Tx,const QByteArray *Rx);

    uint8_t CharToHex(QString text);
    //Structure to handle MBMaster properties
    struct Param {
        uint8_t slaveID;
        QVector<uint8_t> msgCounters=QVector<uint8_t>(FWU_DispMaxNAdr,0);
        uint delay = 0;
        uint maxTry = 1;
        bool doLog = false;
        bool linkStat = false;
        uint tBreak1Byte = 1;
        float tOutMult=FWU_DefTOutMult;
    };


    //Property structure holding all MBMaster parameters
    Param m_par;
    //Pointer to log handler
    QFile* m_log = nullptr;
    //Pointer to serial port handler
    COMPort *m_port = nullptr;
    //Mutex dedicated to manage delay time between consecutive transactions
    QMutex m_continue;
    //Wait condition dedicated to handle duration of delay time between consecutive transactions
    QWaitCondition m_pauseManager;
};

#endif // FWMASTER_H
