#ifndef PRINTERAPI_H
#define PRINTERAPI_H

#include <Arduino.h>

class Printer {
public:
  void Set_Printer_Uart(HardwareSerial &SerialData, uint8_t tx, uint8_t rx,
                        uint16_t baud);
  void Printer_Init();
  // void Clear_Buffer();
  void NewLine_Setting(uint8_t n);
  void Print_NewLine(uint8_t n);
  void Print_ASCII(String data);
  void Set_adjlevel(String AdjustLevel);
  void Set_QRCode(String QRCode);
  void Print_QRCode();
  void BarCode_switch(bool state);
  void Set_BarCode_HRI(String Position);
  void Print_BarCode(String BarType, String BarCode);

  HardwareSerial *AtomSerial;
};

#endif
