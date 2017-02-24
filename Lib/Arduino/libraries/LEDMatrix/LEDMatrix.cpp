/**
 * LED Matrix library for http://www.seeedstudio.com/depot/ultrathin-16x32-red-led-matrix-panel-p-1582.html
 * The LED Matrix panel has 32x16 pixels. Several panel can be combined together as a large screen.
 *
 * Coordinate & Connection (Arduino -> panel 0 -> panel 1 -> ...)
 *   (0, 0)                                     (0, 0)
 *     +--------+--------+--------+               +--------+--------+
 *     |   5    |    4   |    3   |               |    1   |    0   |
 *     |        |        |        |               |        |        |<----- Arduino
 *     +--------+--------+--------+               +--------+--------+
 *     |   2    |    1   |    0   |                              (64, 16)
 *     |        |        |        |<----- Arduino
 *     +--------+--------+--------+
 *                             (96, 32)
 *  Copyright (c) 2013 Seeed Technology Inc.
 *  @auther     Yihui Xiong
 *  @date       Nov 8, 2013
 *  @license    MIT
 */

#include <Arduino.h>
#include "LEDMatrix.h"
#ifdef __AVR__
 #include <avr/pgmspace.h>
#else
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

LEDMatrix::LEDMatrix(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t oe, uint8_t r1, uint8_t stb, uint8_t clk)
 {
    this->clk = clk;
    this->r1 = r1;
    this->stb = stb;
    this->oe = oe;
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;

    mask = 0xff;
    state = 0;
    cursor_y  = cursor_x    = 0;
    textsize=1;
    wrap=0;
}

void LEDMatrix::begin(const unsigned char *newfont,uint8_t *displaybuf, uint16_t width, uint16_t height)
{
//    ASSERT(0 == (width % 32));
//    ASSERT(0 == (height % 16));

    this->displaybuf = displaybuf;
    this->width = width;
    this->height = height;

    pinMode(a, OUTPUT);
    pinMode(b, OUTPUT);
    pinMode(c, OUTPUT);
    pinMode(d, OUTPUT);
    pinMode(oe, OUTPUT);
    pinMode(r1, OUTPUT);
    pinMode(clk, OUTPUT);
    pinMode(stb, OUTPUT);

    #if FASTWRITE == 1
    _a = portOutputRegister(digitalPinToPort(a));
    _b = portOutputRegister(digitalPinToPort(b));
    _c = portOutputRegister(digitalPinToPort(c));
    _d = portOutputRegister(digitalPinToPort(d));
    _oe = portOutputRegister(digitalPinToPort(oe));
    _r1 = portOutputRegister(digitalPinToPort(r1));
    _clk = portOutputRegister(digitalPinToPort(clk));
    _stb = portOutputRegister(digitalPinToPort(stb));

    pa = digitalPinToBitMask(a);
    pb = digitalPinToBitMask(b);
    pc = digitalPinToBitMask(c);
    pd= digitalPinToBitMask(d);
    poe = digitalPinToBitMask(oe);
    pr1 = digitalPinToBitMask(r1);
    pclk= digitalPinToBitMask(clk);
    pstb = digitalPinToBitMask(stb);
    #endif



    state = 1;
    setFont(newfont);
}


void LEDMatrix::drawPixel(uint16_t x, uint16_t y, uint8_t pixel,bool direct)
{
  if (x >= width || y >= height ) return;

    uint8_t *byte = (drawbuf && !direct ? drawbuf : displaybuf) + x / 8 + y * width / 8;
    uint8_t  bit = x % 8;

    if (pixel) {
        *byte |= 0x80 >> bit;
    } else {
        *byte &= ~(0x80 >> bit);
    }
}

void LEDMatrix::drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t pixel)
{
    for (uint16_t x = x1; x < x2; x++) {
        for (uint16_t y = y1; y < y2; y++) {
            drawPixel(x, y, pixel);
        }
    }
}

void LEDMatrix::drawImage(uint16_t xoffset, uint16_t yoffset, uint16_t width, uint16_t height, const uint8_t *image)
{
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            const uint8_t *byte = image + (x + y * width) / 8;
            uint8_t  bit = 7 - x % 8;
            uint8_t  pixel = (*byte >> bit) & 1;

            drawPixel(x + xoffset, y + yoffset, pixel);
        }
    }
}

void LEDMatrix::clear()
{
    memset((drawbuf ? drawbuf : displaybuf),0,(width * height / 8));
    setCursor(0,0);
    setTextSize(1);
}

void LEDMatrix::reverse()
{
    mask = ~mask;
}

uint8_t LEDMatrix::isReversed()
{
    return mask;
}

void LEDMatrix::scan()
{
    static uint8_t row = 0;  // from 0 to 15

    if (!state) {
        return;
    }

    uint8_t *head = displaybuf + row * (width / 8);
    for (uint8_t line = 0; line < (height / 16); line++) {
        uint8_t *ptr = head;
        head += width * 2;              // width * 16 / 8

        for (uint8_t byte = 0; byte < (width / 8); byte++) {
            uint8_t pixels = *ptr;
            ptr++;
            pixels = pixels ^ mask;     // reverse: mask = 0xff, normal: mask =0x00
            for (uint8_t bit = 0; bit < 8; bit++) {
                #if FASTWRITE == 1
                *_clk &= ~pclk; //LOW
                if (pixels & (0x80 >> bit)) *_r1 |= pr1; else *_r1 &= ~pr1;
                *_clk |= pclk;
                #else
                digitalWrite(clk, LOW);
                digitalWrite(r1, pixels & (0x80 >> bit));
                digitalWrite(clk, HIGH);
                #endif
            }
        }
    }

    #if FASTWRITE == 1
    *_oe |= poe;             // disable display

    // select row
    if (row & 0x01) *_a |= pa; else *_a &= ~pa;
    if (row & 0x02) *_b |= pb; else *_b &= ~pb;
    if (row & 0x04) *_c |= pc; else *_c &= ~pc;
    if (row & 0x08) *_d |= pd; else *_d &= ~pd;

    // latch data
    *_stb &= ~pstb;
    *_stb |= pstb;
    *_stb &= ~pstb;


    *_oe &= ~poe;              // enable display
    #else
    digitalWrite(oe, HIGH);              // disable display

    // select row
    digitalWrite(a, (row & 0x01));
    digitalWrite(b, (row & 0x02));
    digitalWrite(c, (row & 0x04));
    digitalWrite(d, (row & 0x08));

    // latch data
    digitalWrite(stb, LOW);
    digitalWrite(stb, HIGH);
    digitalWrite(stb, LOW);

    digitalWrite(oe, LOW);              // enable display
    #endif

    row = (row + 1) & 0x0F;
}

void LEDMatrix::on()
{
    state = 1;
}

void LEDMatrix::off()
{
    state = 0;
    digitalWrite(oe, HIGH);
}

// Bresenham's algorithm - thx wikpedia
void LEDMatrix::drawLine(int16_t x0, int16_t y0,int16_t x1, int16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void LEDMatrix::drawFastVLine(int16_t x, int16_t y,
				 int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x, y+h-1, color);
}

void LEDMatrix::drawFastHLine(int16_t x, int16_t y,
				 int16_t w, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x+w-1, y, color);
}
void LEDMatrix::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLine(i, y, h, color);
  }
}


// Draw a character
void LEDMatrix::drawChar(int16_t x, int16_t y, unsigned char c, uint8_t size) {

  if (rotation==0) {
    if((x >= width)            || // Clip right
       (y >= height)           || // Clip bottom
      ((x + (fontW+getFontExtra()) * size - 1) < 0) || // Clip left
      ((y + (fontH+getFontExtra()) * size - 1) < 0))   // Clip top
      return;

    for (int8_t i=0; i<fontW+getFontExtra(); i++ ) {
     uint8_t line;
      if (i == fontW)
        line = 0x0;
      else
       line = pgm_read_byte(font+((c-fontS)*fontW)+i);
      for (int8_t j = 0; j<fontH; j++) {
        if (line & 0x1) {
          if (size == 1) // default size
            drawPixel(x+i, y+j, 1);
          else {  // big size
            fillRect(x+(i*size), y+(j*size), size, size, 1);
          }
        } else  {
          if (size == 1) // default size
            drawPixel(x+i, y+j, 0);
          else {  // big size
            fillRect(x+i*size, y+j*size, size, size, 0);
          }
        }
        line >>= 1;
      }
      if (drawbuf && (i%4==0)) scan();
    }
  } else {
       if((x >= width)            || // Clip right
       (y >= height)           || // Clip bottom
      ((x + (fontH+getFontExtra()) * size - 1) < 0) || // Clip left
      ((y + (fontW+getFontExtra()) * size - 1) < 0))   // Clip top
      return;

    for (int8_t i=0; i<fontW+getFontExtra(); i++ ) {
     uint8_t line;
      if (i == fontW)
        line = 0x0;
      else
       line = pgm_read_byte(font+((c-fontS)*fontW)+i);
      for (int8_t j = 0; j<fontH; j++) {
        if (line & 0x1) {
          if (size == 1) // default size
            drawPixel(x+(fontH-j-1), y+i, 1);
          else {  // big size
            fillRect(x+((fontH-j-1)*size), y+(i*size), size, size, 1);
          }
        } else {
          if (size == 1) // default size
            drawPixel(x+(fontH-j-1), y+i, 0);
          else {  // big size
            fillRect(x+(fontH-j-1)*size, y+(i*size), size, size, 0);
          }
        }
        line >>= 1;
      }
      if (drawbuf  && (i%4==0)) scan();
    }
  }
  if (drawbuf) scan();
}


#if ARDUINO >= 100
size_t LEDMatrix::write(uint8_t c) {
#else
void LEDMatrix::write(uint8_t c) {
#endif
  if (c == '\n') {
    cursor_y += textsize*(getFontHeight()+getFontExtra());
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else {
    drawChar(cursor_x, cursor_y, c, textsize);
    cursor_x += textsize*(getFontWidth()+getFontExtra());
    if (wrap && (cursor_x > (width - textsize*getFontHeight()))) {
        cursor_y += textsize*(getFontHeight()+getFontExtra());
        cursor_x = 0;
    }
  }
#if ARDUINO >= 100
  return 1;
#endif
}


// Draw a rectangle
void LEDMatrix::drawRect(int16_t x, int16_t y,int16_t w, int16_t h, uint16_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

void LEDMatrix::setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
}

void LEDMatrix::setTextSize(uint8_t s) {
  textsize = (s > 0) ? s : 1;
}

void LEDMatrix::setFont(const unsigned char *newfont){
   font=newfont+5;
   fontW=pgm_read_byte(newfont);
   fontH=pgm_read_byte(newfont+1);
   fontS=pgm_read_byte(newfont+2);
   setRotation(pgm_read_byte(newfont+3));
   fontE=pgm_read_byte(newfont+4);
   setTextSize(1);
}

void LEDMatrix::display(){
  if (drawbuf) {
    memcpy(displaybuf,drawbuf,(width * height / 8));
  }
  scan();
}



