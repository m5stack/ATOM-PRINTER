#include <M5Atom.h>
#include "ATOM_PRINTER.h"
#include "bitmap.h"

ATOM_PRINTER printer;

void setup() {
    M5.begin(true, false, true);
    printer.begin();
    M5.dis.drawpix(0, 0x00ffff);
    printer.init();
}

void loop() {
    if(M5.Btn.wasPressed()){
        printer.init();
        printer.printASCII("M5STACK");
        delay(2000);
        printer.init();
        printer.setBarCodeHRI(ABOVE);
        printer.printBarCode(CODE128, "M5STACK");
        delay(2000);
        printer.init();
        printer.printQRCode("M5STACK");
        delay(2000);
        printer.init();
        printer.printBMP(0, 184, 180, bitbuffer2);
    }
    M5.update();
}
