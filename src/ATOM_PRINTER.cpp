#include "ATOM_PRINTER.h"
#include "ATOM_PRINTER_CMD.h"

void ATOM_PRINTER::begin(HardwareSerial *serial, int baud, uint8_t RX,
                         uint8_t TX, bool debug) {
    _debug  = debug;
    _serial = serial;
    _serial->begin(baud, SERIAL_8N1, RX, TX);
}

void ATOM_PRINTER::init() {
    _serial->write(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
}

void ATOM_PRINTER::cleanBuffer() {
    for (int i = 0; i < 256; i++) {
        buffer[i] = 0;
    }
}

void ATOM_PRINTER::newLine(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        _serial->write(0x0A);
    }
}

void ATOM_PRINTER::printASCII(String data) {
    _serial->print(data);
}

void ATOM_PRINTER::printQRCode(String qrcode) {
    // set qrcode
    uint8_t len, nH, nL;
    len = qrcode.length();
    len += 3;
    nL = len & 0x00ff;
    nH = (len >> 8) & 0x00ff;
    cleanBuffer();
    memcpy(buffer, SET_QRCODE_CMD, sizeof(SET_QRCODE_CMD));
    buffer[3] = nL;
    buffer[4] = nH;
    _serial->write(buffer, sizeof(SET_QRCODE_CMD));
    _serial->print(qrcode);
    _serial->write(0x00);
    // print qrcode
    cleanBuffer();
    memcpy(buffer, PRINTER_QRCODE_CMD, sizeof(PRINTER_QRCODE_CMD));
    _serial->write(buffer, sizeof(PRINTER_QRCODE_CMD));
}

void ATOM_PRINTER::setBarCodeHRI(BarCodePos_t pos) {
    _serial->write(SET_BAR_CODE_POS_CMD, sizeof(SET_BAR_CODE_POS_CMD));
    _serial->write(pos);
}
void ATOM_PRINTER::setQRCodeECL(QRCode_EC_Level_t level) {
    _serial->write(SET_QRCODE_ECL_CODE_CMD, sizeof(SET_QRCODE_ECL_CODE_CMD));
    _serial->write(level);
}

void ATOM_PRINTER::enableBarCode(bool state){
    cleanBuffer();
    memcpy(buffer, ENABLE_BAR_CODE_MODE_CMD, sizeof(ENABLE_BAR_CODE_MODE_CMD));
    buffer[3]   = state;
    _serial->write(buffer, sizeof(ENABLE_BAR_CODE_MODE_CMD));
}

void ATOM_PRINTER::printBarCode(BarCode_t type, String barcode) {
    enableBarCode(1);
    cleanBuffer();
    memcpy(buffer, PRINTER_BAR_CODE_CMD, sizeof(PRINTER_BAR_CODE_CMD));
    uint8_t len = barcode.length();
    buffer[2]   = type;
    buffer[3]   = len;
    _serial->write(buffer, sizeof(PRINTER_BAR_CODE_CMD));
    _serial->print(barcode);
    _serial->write(0x00);
    enableBarCode(0);
}

void ATOM_PRINTER::printBMP(uint8_t mode, uint8_t xdot, uint8_t ydot,
                            uint8_t *buffer) {
    uint8_t tmp;
    uint16_t len;
    if (mode > 3) mode = 3;
    memcpy(buffer, PRINTER_BMP_CMD, sizeof(PRINTER_BMP_CMD));
    xdot = xdot / 8;
    buffer[3]   = mode;
    buffer[4]   = (uint8_t)(xdot & 0x00ff);
    buffer[5]   = (uint8_t)((xdot >> 8) & 0x00ff);;
    buffer[6]   = (uint8_t)(ydot & 0x00ff);;
    buffer[7]   = (uint8_t)((ydot >> 8) & 0x00ff);;
    _serial->write(buffer, sizeof(PRINTER_BMP_CMD));
    len = xdot * ydot;
    while (len) {
        _serial->write(*buffer++);
        len--;
    }
}
