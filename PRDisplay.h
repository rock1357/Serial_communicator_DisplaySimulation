#ifndef PRDISPLAY_H
#define PRDISPLAY_H

#include <QMainWindow>

#include <QAction>
#include <QLabel>
#include <QSettings>
#include <About.h>
#include <StateMachine.h>
#include <ConnectionWidget.h>
#include <FWMaster.h>

//#define REG_SOFT_VERS 38

QT_BEGIN_NAMESPACE
namespace Ui { class PRDisplay; }
QT_END_NAMESPACE

class PRDisplay : public QMainWindow
{
    Q_OBJECT
public:

    PRDisplay(QWidget *parent = nullptr);
    ~PRDisplay();

signals:
    void SetLCDnumbers(FWUtils::LCD LCDPos,uint8_t slaveID=0,QString leText="");
    void LightOnClicked(uint8_t SlaveID,bool On=true);
    void LightOffClicked(uint8_t SlaveID, bool Off=false);
    void SetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint8 qui8);
    void SetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint16 qui16);
    void SetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,quint32 qui32);
    void SetRegButtonClicked(uint8_t SlaveID,uint16_t RegAdr,QByteArray QBA);
    void GetRegButtonClicked(uint8_t SlaveID, uint16_t RegAdr);
    void SetRegAdrs(int RegIndex);
    void SlaveIdChanged(int PMAddress);
    void SetCounterAction(uint8_t slaveId);
    void ActDecPoint();

public slots:
    void OnShowStatus(QString Status );
    void OnShowEXCP(QString Excp);
    void OnShowRegValues(QString str,QByteArray values);

protected:
    void paintEvent(QPaintEvent *event) override;


private:
    Ui::PRDisplay *ui;

    void initUI();
    void initTB();
    void initStateMachine();
    void Tab1Connections();
    void Tab2Connections();
    void InitSMThread();

    QThread* m_SMThread_M=nullptr;
    About* m_About=nullptr;
    QToolBar* m_toolBar=nullptr;
    ConnectionWidget* m_CW=nullptr;
    QString m_prgTitle="";
    StateMachine* m_SM_M=nullptr;
    int m_Datalength=0;
    uint8_t m_LCDpos=0;
    uint8_t m_slaveID=0;

    void saveSettings();
    void loadSettings();

    int lookUpTable[9]={-1,38,146,148,150,152,154,156,158};

};
#endif // PRDISPLAY_H
