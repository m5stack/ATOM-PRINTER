//初始化打印机
const uint8_t INIT_PRINTER_CMD[] = {0x1B, 0x40};

const uint8_t PRINT_POS_CMD[] = {0x1B, 0x24};

const uint8_t FONT_SIZE_CMD[] = {0x1D, 0x21};

const uint8_t SET_BAR_CODE_POS_CMD[] = {0x1D, 0x48};

const uint8_t ENABLE_BAR_CODE_MODE_CMD[] = {0x1D, 0x45, 0x43, 0xff};

const uint8_t PRINTER_BAR_CODE_CMD[] = {0x1D, 0x6B, 0x41, 0xff};

const uint8_t PRINTER_QRCODE_CMD[] = {0x1D, 0x28, 0x6B, 0x03, 0x00,
                                      0x31, 0x51, 0x30, 0x00};

const uint8_t SET_QRCODE_CMD[] = {0x1D, 0x28, 0x6B, 0xff,
                                  0Xff, 0x31, 0x50, 0x30};

const uint8_t SET_QRCODE_ECL_CODE_CMD[] = {0x1D, 0x28, 0x6B, 0x03,
                                           0X00, 0x31, 0x45};

const uint8_t PRINTER_BMP_CMD[] = {0x1D, 0x76, 0x30, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
