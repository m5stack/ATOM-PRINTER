#include "PrinterApi.h"

void Printer::Set_Printer_Uart(HardwareSerial &SerialData, uint8_t tx,
                               uint8_t rx, uint16_t baud) {
  AtomSerial = &SerialData;
  AtomSerial->begin(baud, SERIAL_8N1, rx, tx);
}

// void Printer::Clear_Buffer()
// {
//     AtomSerial->write(0x1B);
//     AtomSerial->write(0x36);
//     AtomSerial->write(0x00);
// }

void Printer::NewLine_Setting(uint8_t n) {
  AtomSerial->write(0x1B);
  AtomSerial->write(0x23);
  AtomSerial->write(0x23);
  AtomSerial->write(0x42);
  AtomSerial->write(0x4D);
  AtomSerial->write(0x55);
  AtomSerial->write(0x4C);
  AtomSerial->write(n);
}

void Printer::Printer_Init() {
  AtomSerial->write(0x1B);
  AtomSerial->write(0x40);
}

void Printer::Print_NewLine(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    AtomSerial->write(0x0A);
  }
}

void Printer::Print_ASCII(String data) { AtomSerial->print(data); }

void Printer::Set_adjlevel(String AdjustLevel) {
  uint8_t lev;
  if (AdjustLevel == "L")
    lev = 0x48;
  else if (AdjustLevel == "M")
    lev = 0x49;
  else if (AdjustLevel == "Q")
    lev = 0x50;
  else if (AdjustLevel == "H")
    lev = 0x51;

  AtomSerial->write(0x1D);
  AtomSerial->write(0x28);
  AtomSerial->write(0x6B);
  AtomSerial->write(0x03);
  AtomSerial->write(0x00);
  AtomSerial->write(0x31);
  AtomSerial->write(0x45);
  AtomSerial->write(lev);
}
void Printer::Set_QRCode(String QRCode) {
  uint8_t len, nH, nL;
  len = QRCode.length();
  len += 3;
  nL = len & 0x00ff;
  nH = (len >> 8) & 0x00ff;
  AtomSerial->write(0x1D);
  AtomSerial->write(0x28);
  AtomSerial->write(0x6B);
  AtomSerial->write(nL);
  AtomSerial->write(nH);
  AtomSerial->write(0x31);
  AtomSerial->write(0x50);
  AtomSerial->write(0x30);
  AtomSerial->print(QRCode);
  AtomSerial->write(0x00);
}
void Printer::Print_QRCode() {
  AtomSerial->write(0x1D);
  AtomSerial->write(0x28);
  AtomSerial->write(0x6B);
  AtomSerial->write(0x03);
  AtomSerial->write(0x00);
  AtomSerial->write(0x31);
  AtomSerial->write(0x51);
  AtomSerial->write(0x30);
  AtomSerial->write(0x00);
}

void Printer::BarCode_switch(bool state) {
  AtomSerial->write(0x1D);
  AtomSerial->write(0x45);
  AtomSerial->write(0x43);
  AtomSerial->write(state);
}

void Printer::Set_BarCode_HRI(String Position) {
  uint8_t pos;
  if (Position == "Hide")
    pos = 0x00;
  else if (Position == "Above")
    pos = 0x01;
  else if (Position == "Below")
    pos = 0x02;
  else if (Position == "Both")
    pos = 0x03;
  AtomSerial->write(0x1D);
  AtomSerial->write(0x48);
  AtomSerial->write(pos);
}

void Printer::Print_BarCode(String BarType, String BarCode) {
  uint8_t type, len;
  if (BarType == "UPC-A")
    type = 0x41;
  else if (BarType == "UPC-E")
    type = 0x42;
  else if (BarType == "JAN13(EAN13)")
    type = 0x43;
  else if (BarType == "JAN8(EAN8)")
    type = 0x44;
  else if (BarType == "CODE39")
    type = 0x45;
  else if (BarType == "ITF")
    type = 0x46;
  else if (BarType == "CODABAR")
    type = 0x47;
  else if (BarType == "CODE93")
    type = 0x48;
  else if (BarType == "CODE128")
    type = 0x49;

  len = BarCode.length();
  Serial.println(len);
  AtomSerial->write(0x1D);
  AtomSerial->write(0x6B);
  AtomSerial->write(type);
  AtomSerial->write(len);
  AtomSerial->print(BarCode);
  AtomSerial->write(0x00);
}
