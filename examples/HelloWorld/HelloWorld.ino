/*
  Please install the following dependent libraries before compiling:
  M5Atom: https://github.com/m5stack/M5Atom
  FastLED: https://github.com/FastLED/FastLED
*/

#include "PrinterApi.h"
#include <M5Atom.h>


Printer atomPrinter;
HardwareSerial AtomSerial(1);

void setup() {
  M5.begin(true, false, true);

  atomPrinter.Set_Printer_Uart(AtomSerial, 23, 33, 9600);
  atomPrinter.Printer_Init();
  atomPrinter.NewLine_Setting(0x0A);
}

void loop() {
  atomPrinter.Printer_Init();
  atomPrinter.Print_ASCII("Hello M5stack");
  atomPrinter.Print_NewLine(2);
  atomPrinter.Printer_Init();
  atomPrinter.Set_adjlevel("L");
  atomPrinter.Set_QRCode("https://www.M5Stack.com");
  atomPrinter.Print_QRCode();
  atomPrinter.Printer_Init();
  atomPrinter.BarCode_switch(1);
  atomPrinter.Set_BarCode_HRI("Below");
  atomPrinter.Print_BarCode("CODE128", "123456789ABC");
  atomPrinter.BarCode_switch(0);
  atomPrinter.Print_NewLine(3);
  atomPrinter.Printer_Init();
  delay(10000);
}
