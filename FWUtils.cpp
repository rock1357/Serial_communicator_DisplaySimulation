#include "FWUtils.h"

uint8_t FWUtils::CalcCrc8(const QByteArray &Buff)
{
    // Calcolo del controllo CRC sui primi 6 byte del messaggio
    uint8_t crc;

    crc = (uint8_t)(Buff.at(DSP_iStartB));
    for (uint8_t i = 1; i < FWU_MsgLengthNoCrc; i++)
    {
        crc ^= (uint8_t)(Buff.at(i));
    }

    return crc;
}

//§M
QString FWUtils::Buff2Str(const QByteArray &Buff)
{
    QString Str;
    for(const char &ch : Buff) {
        QString c=QString::number(((uint8_t)ch),16).toUpper();
        if(((uint8_t)ch)<0x10) c.prepend("0");
        Str.append(c);
    }
    return Str;
}

QMap<uint8_t, QString> FWUtils::hexToCharMap=
{
        // Map hexadecimal values to their corresponding characters
        {0, "0"}, {1, "1"}, {2, "2"}, {3, "3"},
        {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"},
        {8, "8"}, {9, "9"}, {10, "-"}, {11, "E"},
        {12, "H"}, {13, "L"}, {14, "P"}, {15, " "},
        {16,"."}
};

// Function to split a byte into its most significant and least significant parts
QString FWUtils::byteToString(uint8_t byte)
{
    // Split the byte into its most significant and least significant nibbles
    uint8_t mostSignificant  = byte >> 4;      // Get the most significant part by shifting the byte 4 bits to the right
    uint8_t leastSignificant = byte & 0x0F;   // Get the least significant part by performing a bitwise AND operation with 0x0F (00001111 in binary)

    // Convert each part to its corresponding character
    QString mostSignificantChar  = hexToCharMap.value(mostSignificant); // Look up the most significant part in the charMap
    QString leastSignificantChar = hexToCharMap.value(leastSignificant); // Look up the least significant part in the charMap

    // Return the two characters concatenated together
    return mostSignificantChar + leastSignificantChar;        
}

bool FWUtils::DispStringValidator(const QString &str) {

    bool ok = true; // Assuming 'ok' is initialized somewhere in your code
    QString rule = "^[-HELPhelp\\d\\. ]{%1}$";
    QRegularExpression re;

    if (str.contains(".")) {
        if (str.count(".") > 1) {
            ok = false;
        } else {
            re.setPattern(rule.arg(7));
        }
    } else {
        re.setPattern(rule.arg(6));
    }

    if (ok) {
        QRegularExpressionMatch match = re.match(str);
        if (!match.hasMatch()) {
            ok = false;
        }
    }

    return ok;
}
