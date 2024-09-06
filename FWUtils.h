#ifndef FWUTILS_H
#define FWUTILS_H

#include <QRegularExpression>
#include <QObject>
#include <QMap>

#define FWU_DSP 0
#define FWU_PM  1
#define FWU_START_BIT '@'
#define FWU_STOP_BIT  '*'
#define FWU_CHECKDSP  0x07
#define FWU_RETRY     10

#define FWU_DefNID ((uint8_t)0x00)        // Default Node ID value for Slave
#define FWU_DefTOutMult (2U)               // Default multiplier for communication timeout interval for COM port
#define FWU_DefLog true                    // Default setting regarding the writing of a log file containing all transactions of a session

#define FWU_PMMaxNAdr (256U)                 // Maximum value for the address of any type, since the maximum number of registers of any type is 256
#define FWU_DispMaxNAdr (16U)

#define FWU_MaxNReg /* §M */               // Maximum number of registers that can be set or get with a single FWay message

#define FWU_CmndLen 8                      // Fixed length in bytes of a valid FWay command, including CRC
#define FWU_RspsLen 8                      // Fixed length in bytes of a valid FWay response, including CRC
#define FWU_LCDTextLength 6
#define FWU_MsgLengthNoCrc 6               // Length of message without CRC

class FWUtils : public QObject
{
    Q_OBJECT
public:

    // Enumerator for FW-commands
    enum CID: uint8_t {
        NoCID     =0x00, // Custom: placeholder for no valid Command ID
        Get_Reg   =0x01, // Command to get the register values
        Set_Reg   =0x02, // Command to set the register values
        Write_Top =0x01, // Command to write the value on the top screen
        Write_Mdl =0x02, // Command to write the value on the middle screen
        Write_Btm =0x03, // Command to write the value on the bottom screen
        Check_LCD =0x04, // Command to check the display and counter status
        Light_On  =0x05, // Command to turn on the screen light
        Light_Off =0x06, // Command to turn off the screen light
    };

    // Enumerator for exception managing
    enum EXCP: uint8_t {
        NoExcp      = 0x00,     // No exception raised
        BadRsp      = 0x01,     // Something in the RX buffer was not correct
        Offline     = 0x03,     // The slave is disconnected
        BadMsg      = 0x04,     // Something in the TX buffer was not correct
        WrongAdrs   = 0x05,     // The Display/PM address does not exist
    }; Q_ENUM(EXCP);

    // Enumerator for buffer indices related to display
    enum DSP_BuffNdx : uint8_t {
        // COMMON
        DSP_iStartB    = 0, // Start byte index for PM/Display communication protocol
        DSP_iCRC       = 6, // CRC byte index for PM/Display
        DSP_iStopB     = 7, // Stop byte index for PM/Display
        // COMMAND
        DSP_iCID       = 1, // Tx buffer byte index for write-on-display-screen
        DSP_iData1     = 2, // Tx buffer index for the first couple of values to be written on the display
        DSP_iData2     = 3, // Tx buffer index for the second couple of values to be written on the display
        DSP_iData3     = 4, // Tx buffer index for the third couple of values to be written on the display
        DSP_iCNT       = 2, // Tx buffer index for the activation of the counter
        DSP_iCntMsg    = 3,
        DSP_iADR       = 5, // Tx buffer index for the display address
        DSP_iPP        = 5,
        // RESPONSE
        DSP_iDStat     = 2, // Rx buffer index of the checked display status
        DSP_iCStat     = 3, // Rx buffer index of the checked PM status
        DSP_iMasterAdr = 4  // Rx buffer index address of the Slave
    }; Q_ENUM(DSP_BuffNdx);

    // Enumerator for buffer indices related to PM
    enum PM_BuffNdx: uint8_t {
        // COMMON
        PM_iStartB  = 0, // Start byte index for the PM/Display communication protocol
        PM_iADR     = 5, // Address byte index for the PM/Display
        PM_iCRC     = 6, // CRC byte index for the PM/Display
        PM_iStopB   = 7, // Stop byte index for PM/Display
        // COMMAND
        PM_iCID     = 1, // PM Tx buffer index for Read or Write command
        PM_iMSReg   = 2, // PM Tx buffer index for the most significant part of the uint16 Reg-Address
        PM_iLSReg   = 3, // PM Tx buffer index for the least significant part of the uint16 Reg-Address
        PM_iDTWrite = 4, // PM Tx buffer index for the data to write byte
        // RESPONSE
        PM_iDTRead  = 4, // PM Rx buffer index for the data to be read
    }; Q_ENUM(PM_BuffNdx);

    enum LCD: uint8_t {
        Top=0,
        Mdl,
        Btm,
    };

    // Inner thread structure to accept the serial communication referred to a particular node
    struct RxEngine {
        QMap<uint16_t, uint8_t> RegMap;     // Registers
        QStringList lcds;
        QByteArray rxBuff;                  // Buffer for reception
        uint8_t name = FWU_DSP;             // Name property to distinguish between PM and Display nodes
        uint8_t node = 0x00;                // Display/PM node address
        uint8_t cntMsg = 0;
        uint32_t counter = 0;
        bool Light =0;
        bool DispStatus = true;             // Display property for checking its status
        bool CounterStatus = true;          // Display property for checking its counter status
        RxEngine() {}                       // Default Constructor
        RxEngine(uint8_t n, uint8_t Name): node(n), name(Name) {}; // Constructor with a name identifier to distinguish between display and PM
        RxEngine(uint8_t n, uint8_t Name, QMap<uint16_t,uint8_t> Registers) : node(n), name(Name), RegMap(Registers){};
        RxEngine(uint8_t n, uint8_t Name, QList<QString> InitLCDscreen ): node(n), name(Name), lcds(InitLCDscreen) {};
    };

    static bool DispStringValidator(const QString &str);


    // Map hexadecimal values to their corresponding characters
    static QMap<uint8_t, QString> hexToCharMap;
    // Method for CRC calculation
    static uint8_t CalcCrc8(const QByteArray &Buff);
    //
    static QString byteToString(uint8_t byte);
    //
    static QString Buff2Str(const QByteArray &Buff);
};

#endif // FWUTILS_H
