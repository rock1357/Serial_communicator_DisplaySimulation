#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <QObject>
#include <FWMaster.h>

class StateMachine : public QObject
{
    Q_OBJECT
public:
    explicit StateMachine(ConnectionWidget *CW);

public slots:

    void OnSetLCDnumbers(FWUtils::LCD LCDPos, uint8_t slaveID, QString string="");
    void OnSetCounterActivation(uint8_t slaveID);
    void OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint8  qui8);
    void OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint16 qui16);
    void OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint32 qui32);
    void OnSetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,QByteArray Value);
    void OnGetRegButtonClicked(uint8_t SlaveID, uint16_t RegAdr);
    void onTabChanged(int tabIndex);
    void ThrowEXCP (FWUtils::EXCP err, int n=0);
    void OnLightOnOffClicked(uint8_t slaveID, bool OffOn);

signals:
    void ShowStatus(QString Status);
    void ShowException(QString Excp);
    void ShowRegValues(QString status,QByteArray values=nullptr);
private:
    void updateDisplayStatus();

    int m_TabIndex;
    FWMaster* m_FwM=nullptr;
    QTimer* m_pollTim=nullptr;
    uint8_t m_LCDAdrs=0;
    int m_GetRegAdrs=0;
    QByteArray m_SetValues;
    QByteArray m_GetValues;
    int m_DataLength=2;
    bool m_DStatus=0;
    bool m_CStatus=0;

};

#endif // STATEMACHINE_H
