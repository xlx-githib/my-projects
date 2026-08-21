#ifndef __TEST_RGB565_OR_JPEG
#define __TEST_RGB565_OR_JPRG

#include "key.h"
#include "lcd.h"
#include "ov2640.h"
#include "dcmi_app.h"
#include "dcmi.h"

void rgb565_test(void);
void jpeg_test(void);
uint8_t test_choose(void);
void test_mode(uint8_t ov_mode);

#endif
