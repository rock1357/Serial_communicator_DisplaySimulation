#include "StateMachine.h"
#include <QTextEdit>


StateMachine::StateMachine(ConnectionWidget *CW)
    : QObject()
{
    // instantiate a FWMaster pointer
    m_FwM=new FWMaster(this,CW);

    m_FwM->setLog(true);

    //Set a delay that control the intertime between consecutive transaction (see doTransaction in FWMaster.cpp)
    m_FwM->setDelay(100), m_FwM->setMaxTry(1);

    //Create polling timer that periodically check the status of the Display
    m_pollTim= new QTimer(this);
    m_pollTim->setTimerType(Qt::PreciseTimer);
    m_pollTim->setSingleShot(false);
    m_pollTim->setInterval(1000000);

    //Connect polling timer to slot to update parameters from PM3
    connect(m_pollTim,&QTimer::timeout,this,&::StateMachine::updateDisplayStatus,Qt::DirectConnection);

    connect(m_FwM, &FWMaster::linkFeedback, this, [&](bool linkStat) {
        if (linkStat) {
            // Connected: start polling
            m_pollTim->start();
        } else {
            // Disconnected: stop polling
            m_pollTim->stop();
        }
    });

}

// Manage the tabs use
void StateMachine::onTabChanged(int tabIndex) {
    m_TabIndex=tabIndex;
}

// Manage the choice of the LCDs in the Display and send to it the string which holds the message to be displaied
void StateMachine::OnSetLCDnumbers(FWUtils::LCD LCDPos,uint8_t slaveID,QString string){
    m_FwM->setSlaveID(slaveID);

    FWUtils::EXCP err=FWUtils::NoExcp;
    err=m_FwM->SetLCD(LCDPos,string);

    int n=4;
    ThrowEXCP(err,n);
};

void StateMachine::OnLightOnOffClicked(uint8_t slaveID,bool OffOn){
    m_FwM->setSlaveID(slaveID);

    FWUtils::EXCP err;
    int n;
    if(OffOn==1){
        n=5;
    } else{
        n=6;
    }

    err=m_FwM->SetBackLight(OffOn);
    ThrowEXCP(err,n);
};


// The following function send the quint data to the FWMaster method which has the role to construct the TxBuffer for the chosen register. The one that is selected depends on the
// size of the quint which holds the data to be sent
void StateMachine::OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint8 qui8) {
    FWUtils::EXCP err = FWUtils::NoExcp;
    m_FwM->setSlaveID(SlaveID);

    // Check for the invalid register address case
    if (RegAdr<0) {
        err = FWUtils::WrongAdrs;
        ThrowEXCP(err);
        return;  // Stop processing further if the register address is invalid
    }else {
        err = m_FwM->SetReg(RegAdr, qui8);
        if(err==FWUtils::NoExcp){
            m_SetValues.resize(1);
            m_SetValues[0] = qui8;
            m_DataLength = m_SetValues.length();
            int n = 1;
            ThrowEXCP(err, n);
        } else {
            ThrowEXCP(err);
        }

    }
};

void StateMachine::OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint16 qui16) {
    FWUtils::EXCP err = FWUtils::NoExcp;

    m_FwM->setSlaveID(SlaveID);
    // Check for the invalid register address case
    if (RegAdr <0) {
        err = FWUtils::WrongAdrs;
        ThrowEXCP(err);
        return;  // Stop processing further if the register address is invalid
    }

    //if the previous check was good then send the data to be registered in the valid address of the PM3
    err = m_FwM->SetReg(RegAdr, qui16);
    if(err==FWUtils::NoExcp){
        m_SetValues.resize(2);
        m_SetValues[0] = (quint8)((qui16 & 0xFF00) >> 8);
        m_SetValues[1] = (quint8)(qui16 & 0x00FF);
        m_DataLength = m_SetValues.length();
        int n = 1;
        ThrowEXCP(err, n);
    } else {
        ThrowEXCP(err);
    }
};

void StateMachine::OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint32 qui32) {
    FWUtils::EXCP err = FWUtils::NoExcp;
    m_FwM->setSlaveID(SlaveID);

    // Check for the invalid register address case
    if (RegAdr<0) {
        err = FWUtils::WrongAdrs;
        ThrowEXCP(err);
        return;  // Stop processing further if the register address is invalid
    }

    err = m_FwM->SetReg(RegAdr, qui32);
    //if the previous check was good then send the data to be registered in the valid address of the PM3
    if(err==FWUtils::NoExcp){
        m_SetValues.resize(4);
        m_SetValues[0] = (quint8)((qui32 & 0xFF000000) >> 24);
        m_SetValues[1] = (quint8)((qui32 & 0x00FF0000) >> 16);
        m_SetValues[2] = (quint8)((qui32 & 0x0000FF00) >> 8);
        m_SetValues[3] = (quint8)(qui32 & 0x000000FF);
        m_DataLength = m_SetValues.length();

        int n = 1;
        ThrowEXCP(err, n);
    } else {
        ThrowEXCP(err);
    }
};

void StateMachine::OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,QByteArray value) {
    FWUtils::EXCP err = FWUtils::NoExcp;
    m_FwM->setSlaveID(SlaveID);

    // Check for the invalid register address case
    if (RegAdr<0) {
        err = FWUtils::WrongAdrs;
        ThrowEXCP(err);
        return;  // Stop processing further if the register address is invalid
    }

    err = m_FwM->SetReg(RegAdr, value);
    //if the previous check was good then send the data to be registered in the valid address of the PM3
    if(err==FWUtils::NoExcp){
        m_SetValues = value;
        m_DataLength = m_SetValues.length();

        int n = 1;
        ThrowEXCP(err, n);
    } else {
        ThrowEXCP(err);
    }

};

// In the same manner (meaning: depending on the size of the m_DataLength = m_SetValues.length(); ) the  OnGetRegButtonClicked function will read the data stored in one or more registers depending on the size of the previously sent data
void StateMachine::OnGetRegButtonClicked(uint8_t SlaveID, uint16_t RegAdr) {
    // N.B: The data stored in the chosen register is an uint8.
    FWUtils::EXCP err = FWUtils::NoExcp;
    m_FwM->setSlaveID(SlaveID);
    int n = 2;

    if (m_DataLength == 1) {
        quint8 qui8;
        m_GetValues.resize(m_DataLength);

        // Check for the invalid register address case
        if (RegAdr<0) {
            err = FWUtils::WrongAdrs;
            ThrowEXCP(err);
            return;  // Stop processing further if the register address is invalid
        } else {
            err = m_FwM->GetReg(RegAdr, qui8);
            if(err==FWUtils::NoExcp){
                m_GetValues[0] = qui8;
                ThrowEXCP(err, n);
            } else {
                ThrowEXCP(err);
            }
        }

    } else if (m_DataLength == 2) {
        quint16 qui16 = 0;
        m_GetValues.resize(m_DataLength);
        // Check for the invalid register address case
        if (RegAdr<0) {
            err = FWUtils::WrongAdrs;
            ThrowEXCP(err);
            return;  // Stop processing further if the register address is invalid
        } else {
            err = m_FwM->GetReg(RegAdr, qui16);
            if(err==FWUtils::NoExcp){
                m_GetValues[0] = (quint8)((qui16 & 0x0000FF00) >> 8);
                m_GetValues[1] = (quint8)(qui16 & 0x000000FF);
                ThrowEXCP(err, n);
            } else {
            ThrowEXCP(err);
            }
        }

    } else if (m_DataLength>2 && m_DataLength <= 4) {
        quint32 qui32 = 0;
        m_GetValues.resize(4);

        // Check for the invalid register address case
        if (RegAdr <0) {
            err = FWUtils::WrongAdrs;
            ThrowEXCP(err);
            return;  // Stop processing further if the register address is invalid
        } else {
            err = m_FwM->GetReg(RegAdr, qui32);
            if(err==FWUtils::NoExcp){
                m_GetValues[0] = (quint8)((qui32 & 0xFF000000) >> 24);
                m_GetValues[1] = (quint8)((qui32 & 0x00FF0000) >> 16);
                m_GetValues[2] = (quint8)((qui32 & 0x0000FF00) >> 8);
                m_GetValues[3] = (quint8)(qui32 & 0x000000FF);
                ThrowEXCP(err, n);
            }
            ThrowEXCP(err);
        }
    } else {
        QByteArray GetData=nullptr;
        GetData.resize(m_DataLength);

        // Check for the invalid register address case
        if (RegAdr <0) {
            err = FWUtils::WrongAdrs;
            ThrowEXCP(err);
            return;  // Stop processing further if the register address is invalid
        } else {
            err = m_FwM->GetReg(RegAdr, GetData);
            if(err==FWUtils::NoExcp){
                for(int step=0; step<m_DataLength;++step){
                    m_GetValues[step]=GetData[step];
                }
                ThrowEXCP(err, n);
            }
            ThrowEXCP(err);
        }
    }
};

//Slot connected with the signal of the Button through which we activate the counter of the display
void StateMachine::OnSetCounterActivation(uint8_t slaveID){

    m_FwM->setSlaveID(slaveID);
    // The counter activator is the variable through which we can make the counter to activate. It is defaultly false.
    FWUtils::EXCP err=m_FwM->CheckDisp(true,m_DStatus,m_CStatus);
    if(err!=FWUtils::NoExcp){
        ThrowEXCP(err);
        return;
    }
    int n=3;
    ThrowEXCP(err,n);

};

//Updating the parameters that check if the display and the counter are ok
void StateMachine::updateDisplayStatus(){

    FWUtils::EXCP err=FWUtils::NoExcp;
    if (m_TabIndex==0) {
        for(int i=0;i<1;i++){
            m_FwM->setSlaveID(i);
            err=m_FwM->CheckDisp(false,m_DStatus,m_CStatus);
            if(err==FWUtils::NoExcp){
                int n=3;
                ThrowEXCP(err,n);
            } else{
                ThrowEXCP(err);
            }
        }
    }
};

void StateMachine::ThrowEXCP(FWUtils::EXCP err, int n) {
    switch (err) {
    case FWUtils::NoExcp: {
        if (n == 1) {
            QString status = "Message Sent:";
            emit ShowRegValues(status, m_SetValues);
            m_SetValues.clear();
        } else if (n == 2) {
            QString status = "Message Received:";
            emit ShowRegValues(status, m_GetValues);
            m_GetValues.clear();
        } else if (n == 3) {
            QString status = "Display and Counter states: " + QString(m_DStatus ? "OK" : "NOT OK") + " and " + QString(m_CStatus ? "OK" : "NOT OK");
            emit ShowStatus(status);
        } else if(n==4){
            QString status="message to LCD sent ";
            emit ShowException(status);
        } else if(n==5){
            QString status="Light On";
            emit ShowException(status);
        } else if(n==6){
            QString status="Light Off";
            emit ShowException(status);
        }
        break;
    }
    case FWUtils::BadRsp: {
        // Bad response
        QString status = "Bad response";
        if(m_TabIndex == 0 && n==4){emit ShowException(status);break;};
        (m_TabIndex == 1) ? emit ShowRegValues(status) : emit ShowStatus(status);
        break;
    }

    case FWUtils::WrongAdrs: {
        // Wrong Address
        QString status = "Wrong Address";
        if(m_TabIndex == 0 && n==4){emit ShowException(status);break;};
        (m_TabIndex == 1) ? emit ShowRegValues(status) : emit ShowStatus(status);
        break;
    }
    case FWUtils::Offline: {
        // S.Port Error: Offline
        QString status = "Offline";
        if(m_TabIndex == 0 && n==4){emit ShowException(status);break;};
        (m_TabIndex == 1) ? emit ShowRegValues(status) : emit ShowStatus(status);
        break;
    }
    default: {
        QString status = "Something unexpected happened";
        if(m_TabIndex == 0 && n==4){emit ShowException(status);break;};
        (m_TabIndex == 1) ? emit ShowRegValues(status) : emit ShowStatus(status);
    }
    }
};


