#include "FWMaster.h"
#include <QDir>


FWMaster::FWMaster(QObject *parent, ConnectionWidget *cw)
    : QObject{parent}
{
    //Create serial port handler
    m_port = new COMPort(this);

    //Create log handler
    m_log = new QFile(this);

    //Connect Connection Widget setLink signal to private slot onSetLink
    connect(cw,&ConnectionWidget::setLink,this,&FWMaster::onSetLink,Qt::QueuedConnection);

    //Connect linkFeedback signal to Connection Widget onLinkFeedback slot
    connect(this,&FWMaster::linkFeedback,cw,&ConnectionWidget::onLinkFeedback,Qt::QueuedConnection);

    //Connect setLed signal to Connection Widget onSetLed slot
    connect(this,&FWMaster::setLed,cw,&ConnectionWidget::onSetLed,Qt::QueuedConnection);

}

FWMaster::~FWMaster()
{
    //Close serial port
    m_port->closePort();

    //Close log file
    if(m_log->isOpen()) {
        m_log->close();
    }
}

void FWMaster::onSetLink(const COMPort::Param_t &par)
{
    //Transfer connection parameters to port & save it's current status
    m_par.linkStat = m_port->handlePort(par);
    //Emit signal to update front end
    emit linkFeedback(m_par.linkStat);
    //Manage log file
    manageLog();
}

/* DISPLAY */

//Function to set the LCD command based on the specified address
FWUtils::EXCP FWMaster::SetLCD(FWUtils::LCD lcd, QString &text)
{
    FWUtils::EXCP err=FWUtils::NoExcp;
    QByteArray bufferTx(FWU_CmndLen,0);

    // Add the starter byte and LCD address
    bufferTx[FWUtils::DSP_iStartB]=FWU_START_BIT;
    bufferTx[FWUtils::DSP_iCID]=lcd+1;
    uint8_t PP=0;
    //Parse the text to enstablish if it can be accepted
    if(FWUtils::DispStringValidator(text)){
        text=text.toUpper();
        if(text.contains('.')){
            PP=text.indexOf('.');
            text.removeAt(PP);
            PP=FWU_LCDTextLength-PP;
        }

        //Insert the text (now converted in hex) in the correct bytes of the buffer
        bufferTx[FWUtils::DSP_iData1]=CharToHex(text.mid(0,2));
        bufferTx[FWUtils::DSP_iData2]=CharToHex(text.mid(2,2));
        bufferTx[FWUtils::DSP_iData3]=CharToHex(text.mid(4,2));
        bufferTx[FWUtils::DSP_iADR]=m_par.slaveID;
        if(PP!=0){
            bufferTx[FWUtils::DSP_iADR]|=(0b10000000|((PP&0b0011)<<4));
        }
        bufferTx[FWUtils::DSP_iCRC]=FWUtils::CalcCrc8(bufferTx);
        bufferTx[FWUtils::DSP_iStopB]=FWU_STOP_BIT;

        // Send the command
        err= SendCmnd(bufferTx);
    } else {
        err=FWUtils::BadMsg;
    }

    return err;
}

uint8_t FWMaster::CharToHex(QString text)
{
    uint8_t temp=0;

    temp=FWUtils::hexToCharMap.key(text[0])<<4;
    temp|=FWUtils::hexToCharMap.key(text[1]);

    return temp;
}

FWUtils::EXCP FWMaster::CheckDisp(bool check, bool &DStatus, bool &CStatus)
{
    QByteArray BuffTx(FWU_CmndLen,0);
    QByteArray BuffRx(FWU_RspsLen,0);
    QString TxText;

    FWUtils::EXCP err = FWUtils::NoExcp;

    // Append starter byte and check display request
    BuffTx[FWUtils::DSP_iStartB]=FWU_START_BIT;
    BuffTx[FWUtils::DSP_iCID]=FWUtils::Check_LCD;

    // Append output string to Txbuffer
    TxText = QString("0%1 00 00 0%2").arg(check).arg(m_par.slaveID);
    BuffTx.replace(2,4, QByteArray::fromHex(TxText.toUtf8()));

    BuffTx[FWUtils::DSP_iCntMsg] = m_par.msgCounters[m_par.slaveID]; // Increment the 3rd byte by one to update the counter

    // Append CRC and stop byte
    BuffTx[FWUtils::DSP_iCRC]=FWUtils::CalcCrc8(BuffTx);
    BuffTx[FWUtils::DSP_iStopB]=FWU_STOP_BIT;
    err = SendCmnd(BuffTx, &BuffRx);

    if(err==FWUtils::NoExcp){

        // Check byte containing info for DStatus
        DStatus = (bool)BuffRx.at(FWUtils::DSP_iDStat);

        // Check byte containing info for CStatus
        CStatus = (bool)BuffRx.at(FWUtils::DSP_iCStat);

        m_par.msgCounters[m_par.slaveID]++;

    }
    //Se err è uguale a BadRsp mandare un identico messaggio per un numero di volte uguale a maxtry

    return err;
}

FWUtils::EXCP FWMaster::SetBackLight(bool OnOff)
{
    QByteArray BufferTx;
    BufferTx.resize(FWU_CmndLen);
    BufferTx[FWUtils::DSP_iStartB]=FWU_START_BIT;
    if(OnOff==1){
        BufferTx[FWUtils::DSP_iCID]=FWUtils::Light_On;
    } else {
        BufferTx[FWUtils::DSP_iCID]=FWUtils::Light_Off;
    }
    BufferTx[FWUtils::DSP_iData1]=0;
    BufferTx[FWUtils::DSP_iData2]=0;
    BufferTx[FWUtils::DSP_iData3]=0;
    BufferTx[FWUtils::DSP_iADR]=m_par.slaveID;
    BufferTx[FWUtils::DSP_iCRC]=FWUtils::CalcCrc8(BufferTx);
    BufferTx[FWUtils::DSP_iStopB]=FWU_STOP_BIT;
    FWUtils::EXCP err=FWUtils::NoExcp;
    err= SendCmnd(BufferTx);

    return err;
}

/* PM */

FWUtils::EXCP FWMaster::SetReg(const uint16_t RegAdr, const uint8_t SetData)
{

    FWUtils::EXCP err=FWUtils::NoExcp;
    // Create the transmit buffer
    QByteArray BufferTx(FWU_CmndLen,0);
    // Create a receive buffer
    QByteArray BufferRx(FWU_RspsLen,0);

    // Construct the buffer for transmitting data to the register of the PM2
    BufferTx[FWUtils::DSP_iStartB] = FWU_START_BIT;
    BufferTx[FWUtils::DSP_iCID] = FWUtils::Set_Reg;
    BufferTx[FWUtils::PM_iMSReg] = static_cast<uint8_t>((RegAdr & 0xFF00) >> 8);
    BufferTx[FWUtils::PM_iLSReg] = static_cast<uint8_t>(RegAdr & 0x00FF);
    BufferTx[FWUtils::PM_iDTWrite] = SetData;
    BufferTx[FWUtils::PM_iADR] = m_par.slaveID;
    BufferTx[FWUtils::PM_iCRC] = FWUtils::CalcCrc8(BufferTx);
    BufferTx[FWUtils::PM_iStopB] = FWU_STOP_BIT;

    // Send the command and receive the response
    err=SendCmnd(BufferTx,&BufferRx);

    return err;
}

FWUtils::EXCP FWMaster::SetReg(const uint16_t RegAdr, const uint16_t SetData) {
    FWUtils::EXCP err=FWUtils::NoExcp;

    for (int step = 0; step < 2; ++step) {

        // Extract the 8-bit data part from the 16-bit SetData
        uint8_t Data = static_cast<uint8_t>(SetData >> (8 * step));

        // Write the 8-bit data to the consecutive register
        err = SetReg(RegAdr + step, Data);

        if(err!=FWUtils::NoExcp){
            break;
        }

    }

    // If the loop completes without any error, return NoExcp; else return the excp;
    return err;
}

FWUtils::EXCP FWMaster::SetReg(const uint16_t RegAdr, const uint32_t SetData) {
    FWUtils::EXCP err=FWUtils::NoExcp;

    for (int step = 0; step < 4; ++step) {
        uint8_t Data = static_cast<uint8_t>(SetData >> (8 * step));

        // Write the data byte to the register
        err = SetReg(RegAdr + step, Data);

        if(err!=FWUtils::NoExcp){
            break;
        }

    }

    // If the loop completes without any error, return NoExcp; else return the excp;
    return err;
}

FWUtils::EXCP FWMaster::SetReg(const uint16_t RegAdr,const QByteArray SetData) {
    FWUtils::EXCP err=FWUtils::NoExcp;

    for (int step = 0; step < SetData.length(); ++step) {

        uint8_t Data = static_cast<uint8_t>(SetData[step]);

        // Write the data byte to the register
        err = SetReg(RegAdr + step, Data);

        if(err!=FWUtils::NoExcp){
            break;
        }
    }
    // If the loop completes without any error, return NoExcp; else return the excp;
    return err;
}

FWUtils::EXCP FWMaster::GetReg(const uint16_t RegAdr, uint8_t &GetData)
{
    // Construct the buffer for transmitting data to the register of the PM3
    QByteArray BuffTx(FWU_CmndLen, 0);
    QByteArray BuffRx(FWU_RspsLen, 0);
    BuffTx[FWUtils::PM_iStartB] = FWU_START_BIT;
    BuffTx[FWUtils::PM_iCID] = FWUtils::Get_Reg;
    BuffTx[FWUtils::PM_iADR] = m_par.slaveID;
    BuffTx[FWUtils::PM_iMSReg] = static_cast<uint8_t>((RegAdr & 0xFF00) >> 8);
    BuffTx[FWUtils::PM_iLSReg] = static_cast<uint8_t>(RegAdr & 0x00FF);
    BuffTx[FWUtils::PM_iCRC] = FWUtils::CalcCrc8(BuffTx);
    BuffTx[FWUtils::PM_iStopB] = FWU_STOP_BIT;

    // Send Command
    FWUtils::EXCP err = SendCmnd(BuffTx,&BuffRx);

    if (err==FWUtils::NoExcp) {
        // Retrieve the data byte from the response buffer
        GetData = BuffRx[FWUtils::PM_iDTRead];
    }

    return err;
}

FWUtils::EXCP FWMaster::GetReg(const uint16_t RegAdr, uint16_t &GetData) {
    // Loop over the two bytes to read
    FWUtils::EXCP err=FWUtils::NoExcp;
    for (int step = 0; step < 2; ++step) {
        uint8_t getdataTemp = 0;

        // Call the ReadFromFWReg function to get data from the specified register
        err = GetReg(RegAdr + step, getdataTemp);

        if(err!=FWUtils::NoExcp) break;

        // Combine the received data bytes into the final 16-bit value
        GetData |= static_cast<uint16_t>(getdataTemp) << (8 * step);
    }

    // Return NoExcp if the operation was successful
    return err;
}

FWUtils::EXCP FWMaster::GetReg(const uint16_t RegAdr, uint32_t &GetData) {
    // Loop over the four bytes to read
    FWUtils::EXCP err=FWUtils::NoExcp;
    for (int step = 0; step < 4; ++step) {
        uint8_t getdataTemp = 0;

        // Call the ReadFromFWReg function to get data from the specified register
        err = GetReg(RegAdr + step, getdataTemp);

        // Check for errors during the read operation
        if(err!=FWUtils::NoExcp){
            break;
        }

        // Combine the received data bytes into the final 32-bit value
        GetData |= static_cast<uint32_t>(getdataTemp) << (8 * step);
    }

    // Return NoExcp if the operation was successful
    return err;
}

FWUtils::EXCP FWMaster::GetReg(const uint16_t RegAdr, QByteArray &GetData) {
    // Loop over the specified length of the QByteArray
    FWUtils::EXCP err=FWUtils::NoExcp;
    for (int step = 0; step < GetData.length(); ++step) {
        quint8 getdataTemp = 0;

        // Call the ReadFromFWReg function to get data from the specified register
        err = GetReg(RegAdr + step, getdataTemp);

        // Check for errors during the read operation
        if (err != FWUtils::NoExcp) {
            break;  // Return the error code if any
        }

        // Store the received data in the QByteArray
        GetData[step] = getdataTemp;
    }

    // Return NoExcp if the operation was successful
    return err;
}

/* COMMON */

FWUtils::EXCP FWMaster::SendCmnd(QByteArray &buffTx, QByteArray *buffRx)
{
    FWUtils::EXCP err=FWUtils::NoExcp;

    for(int i=0;i<m_par.maxTry;i++){
        if (doTransaction(buffTx, buffRx)) {
            if(buffRx!=nullptr){
                //Payload-Check
                err=PayLoadCheck(buffTx,buffRx);
                if (err==FWUtils::NoExcp) {
                    //No exception: signal it
                    emit setLed(":/LedGreenOn.png");
                } else {
                    //Exception detected: signal it
                    emit setLed(":/LedRedOn.png");
                }
            } else {
                if (err==FWUtils::NoExcp) {
                    //No exception: signal it
                    emit setLed(":/LedGreenOn.png");
                }
            }
        } else {
            // Transaction failed: check connection
            m_par.linkStat = m_port->openPort();
            // Exception detected: signal it
            if (m_par.linkStat) {
                // System online but transaction failed: update return variable
                err= FWUtils::BadRsp;
            } else {
                // System offline (and of course transaction failed): update return variable
                err= FWUtils::Offline;
            }
            // Update front end with updated link status infos
            emit linkFeedback(m_par.linkStat);
        }

        //If the message has been sent or the connection has been lost then exit from the cycle because ther is nothing more to do.
        if (err==FWUtils::NoExcp || err==FWUtils::Offline) break;
    }

    //Return transaction result
    return err;
}

FWUtils::EXCP FWMaster::PayLoadCheck(const QByteArray &Tx, const QByteArray *Rx) {
    uint8_t NBytesToChek = 3;
    FWUtils::EXCP err = FWUtils::NoExcp;

    switch (Tx.at(FWUtils::DSP_iCID)) {
    case FWUtils::Set_Reg:
        NBytesToChek++;
        Q_FALLTHROUGH();
    case FWUtils::Get_Reg: {
        QByteArray ttx = Tx.mid(1, NBytesToChek);
        QByteArray rrx = Rx->mid(1, NBytesToChek);
        if (ttx != rrx) {
            err = FWUtils::BadRsp;
        } else {
            if (Rx->at(FWUtils::PM_iADR)!=0) {
                err = FWUtils::WrongAdrs;
            }
        }

        break;
    }
    case FWUtils::Check_LCD: {

        if(Rx->at(FWUtils::DSP_iCID)!=FWU_CHECKDSP){
            err = FWUtils::EXCP::BadRsp;
        } else {
            // Clarify the meaning of bytes 4 of the Rx and 5 of the Tx
            uint8_t RespAdr = Rx->at(FWUtils::DSP_iMasterAdr);
            uint8_t CmndAdr = Tx.at(FWUtils::DSP_iADR);

            // Verify if the response command is present
            if (RespAdr != CmndAdr) {
                err = FWUtils::EXCP::BadRsp;
            }
        }

        break;
    }
    default:
        err = FWUtils::BadMsg;
    }

    return err;
}

bool FWMaster::doTransaction(QByteArray &txBuff, QByteArray *rxBuff)
{
    //Set return value (a.k.a. transaction result)
    bool rxReceived = false;

    //Check if a connection was established
    if (m_par.linkStat) {
        // Prepare local rx buffer & parameters for reception
        int len = 0;
        int ndx = -1;
        uint32_t tOut = 0;
        QByteArray rxBuffTemp;
        m_par.tBreak1Byte = qCeil(m_par.tOutMult * m_port->timeout1Byte());

        //Write tx in log file
        if(m_par.doLog) {
            m_log->write("Tx: "+FWUtils::Buff2Str(txBuff).toUtf8()+"\n");
        }

        /* TRANSMISSION */

        qDebug() << "TxBuff=" << txBuff.toHex('-');

        // Check the integrity of messages directed to both the Display OR PM3
        if (!m_port->blockingWrite(txBuff, m_par.tBreak1Byte)) {
            // Unable to write the full command: force timeout & abort transaction
            tOut = 2000;
        } else {
            // Check if the Display message is supposed to receive an answer
            if (rxBuff == nullptr) {
                //If an answer is not expected (a.k.a rxBuff=nullptr) for this kind of message: bypass reception loop
                rxReceived=true;
                tOut = 2000;
            }
        }

        /* RESPONSE */

        // Last character of the message sent: enter the reception loop
        while (tOut < 2000) {
            // Wait for characters to be read
            if (m_port->waitForReadyRead(20)) {
                // Check for available bytes even if waitForReadyRead returns true
                if (m_port->bytesAvailable()) {
                    // Read all received characters
                    rxBuffTemp.append(m_port->readAll());
                    qDebug() << "RxBuff=" << rxBuffTemp.toHex('-');
                    tOut = 0;

                } else {
                    // No error from waitForReadyRead, but no characters read: increment timeout
                    tOut += 20;
                }
            } else {
                // Error from waitForReadyRead: manage it
                QSerialPort::SerialPortError err = m_port->error();
                switch (err) {
                case QSerialPort::TimeoutError:
                    // Timeout error: increment timeout
                    tOut += 20;
                    break;
                case QSerialPort::NoError:
                    // No error from the serial port: do nothing
                    break;
                default:
                    // Error during reception: abort transaction
                    tOut=2000;
                    break;
                }
                // Clear port errors for the next transaction
                m_port->clearError();
            }

            if (rxBuffTemp.length() > len) {

                // Synchronize with the first character
                if (ndx != 0) {
                    // Find the index of the starter byte inside the buffer
                    ndx = rxBuffTemp.indexOf(FWU_START_BIT);
                    if (ndx > 0) {
                        // Starter byte found: sync buffer
                        rxBuffTemp.remove(0, ndx);
                        ndx = 0;
                    } else if(ndx==-1){
                        rxBuffTemp.clear();
                        tOut+=20;
                        continue;
                    }
                }

                // Update length
                len = rxBuffTemp.length();
                // Check synchronization with the minimum response length
                if (len >= FWU_RspsLen) {
                    if (rxBuffTemp.at(FWUtils::DSP_iStopB) == FWU_STOP_BIT) {
                        // Synchronize with the last character
                        // & shift local rx buffer into the proper rx buffer
                        *rxBuff=rxBuffTemp.mid(0,FWUtils::DSP_iStopB);
                        rxBuffTemp=rxBuffTemp.mid(FWUtils::DSP_iStopB);

                        uint8_t CalculatedCRC = FWUtils::CalcCrc8(*rxBuff);
                        uint8_t InputCRC = (*rxBuff).at(FWUtils::DSP_iCRC);

                        // Verify if the CRC of the reply is the same as the calculated one
                        if (InputCRC == CalculatedCRC) {
                            //write rx in log file
                            if(m_par.doLog) {
                                m_log->write("Rx: "+FWUtils::Buff2Str(*rxBuff).toUtf8()+"\n");
                            }
                            // Response received: force timeout
                            tOut = 2000;
                            rxReceived=true;
                        } else {
                            //Crc or response check fail: try reparse
                            (*rxBuff).clear();
                            tOut+=20;
                            len = 0;
                            ndx = -1;
                        }
                    } else {
                        //Check if the message is shifted by one because the first @ is a mistake
                        rxBuffTemp.removeFirst();
                        ndx=-1;
                        len=0;
                        tOut+=20;
                    }
                } else {
                    tOut+=20;
                }
            }
        }
    }

    // Clear port buffer in & out
    m_port->clear();

    // Apply delay between consecutive transactions (if needed)
    if (m_par.delay) {
        m_pauseManager.wait(&m_continue, m_par.delay);
    }

    //response timeout reached or no link established: transaction failed
    return rxReceived;
}

//Open (overwrite) or close log file based on current connection status
void FWMaster::manageLog()
{
    //Check if the port has received a disconnect command
    if(m_port->portName() != "") {
        //Check if connection is active and if log is enabled
        if((m_par.linkStat)&&(m_par.doLog)){
            //Log needs to be opened: check it
            if(!m_log->isOpen()) {
                //Set Log file name
                m_log->setFileName(QDir::currentPath()+QString("/data/%1_Log.txt").arg(m_port->param().name));
                //Check if Log exist already
                if(m_log->exists()){
                    //Log already exist:remove it
                    m_log->remove();
                }
                //Open Log
                m_log->open(QIODeviceBase::WriteOnly);
            }
        }
    } else {
        //Log needs to be cloased: check it
        if(m_log->isOpen()) {
            //Close Log
            m_log->close();
        }
    }
}



