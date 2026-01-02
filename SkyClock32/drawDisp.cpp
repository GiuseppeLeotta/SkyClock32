
#include "drawDisp.h"
#include <pgmspace.h>


#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))

inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
  return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
}

inline uint8_t *pgm_read_bitmap_ptr(const GFXfont *gfxFont) {
  return (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);
}

void drawDisp::setColor(uint16_t _color, uint16_t _bkcolor) {
  color[pBuff] = _color;
  bkcolor = _bkcolor;
}


void drawDisp::clrbuff(void) {
  pBuff = nBuff = 0;
  for (int i = 0; i < MAXCANVAS; i++) {
    buff[i] = NULL;
    ox[i] = oy[i] = lx[i] = ly[i] = stride[i] = 0;
    priority[i] = -1;
  }
}

drawDisp::drawDisp(display *_disp, const GFXfont *gfx_Font, uint16_t _color, uint16_t _bkcolor, const char *_charReduction) {
  baseline = lyfont = 0;
  disp = _disp;
  charReduction = _charReduction;
  clrbuff();
  setColor(_color, _bkcolor);
  setFont(gfx_Font);
}

drawDisp::drawDisp(display *_disp, const GFXfont *gfx_Font, uint16_t _color, uint16_t _bkcolor, uint16_t _lx, uint16_t _ly, const char *_charReduction) {
  drawDisp(_disp, gfx_Font, _color, _bkcolor, _charReduction);
  creatCanvas(_lx, _ly);
}

drawDisp::~drawDisp() {
  for (int i = 0; i < MAXCANVAS; i++)
    if (buff[i]) delete (buff[i]);
}


void drawDisp::setPixel(uint16_t _x, uint16_t _y, bool v) {

  if (!buff[pBuff]) return;
  uint16_t x = _x - ox[pBuff];
  uint16_t y = _y - oy[pBuff];
  if (x >= lx[pBuff] || y >= ly[pBuff]) return;
  uint32_t idx = y * stride[pBuff] + (x >> 3);
  uint8_t mask = 0x80 >> (x & 7);
  if (v) buff[pBuff][idx] |= mask;
  else buff[pBuff][idx] &= ~mask;
}

bool drawDisp::getPixel(uint16_t _x, uint16_t _y) {
  if (!buff[pBuff]) return false;
  uint16_t x = _x - ox[pBuff];
  uint16_t y = _y - oy[pBuff];
  if (x >= lx[pBuff] || y >= ly[pBuff]) return false;
  uint32_t idx = y * stride[pBuff] + (x >> 3);
  uint8_t mask = 0x80 >> (x & 7);
  if (buff[pBuff][idx] & mask) return true;
  return false;
}

void drawDisp::setFont(const GFXfont *gfx_Font) {
  gfxFont = gfx_Font;
  GFXglyph *glyph;
  yAdvance = pgm_read_byte(&gfxFont->yAdvance);
  nChar = pgm_read_byte(&gfxFont->last) - pgm_read_byte(&gfxFont->first);
  int16_t mm = 0, mx = 0;
  int8_t yo;
  for (uint8_t i = 0; i < nChar; i++) {
    glyph = pgm_read_glyph_ptr(gfxFont, i);
    yo = pgm_read_byte(&glyph->yOffset);
    mm = (mm > yo ? yo : mm);
    mx = (mx < yo ? yo : mx);
  }
  baseline = -mm;
  lyfont = mx - mm + 1;
  //Serial.printf("mm=%d mx=%d n=%d bs=%d ly=%d\n", mm, mx, nChar, baseline, lyfont);
}

void drawDisp::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t thickness, int16_t lenDash) {
  if (thickness < 1) return;  // Spessore minimo 1 pixel

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  int count = 0;
  bool onDash = true;

  // Caso speciale: linea di spessore 1 (comportamento originale)
  if (thickness == 1) {
    while (1) {
      if (onDash) setPixel(x0, y0);
      if (x0 == x1 && y0 == y1) break;

      if (lenDash) {
        count++;
        if (count == lenDash) {
          count = 0;
          onDash = !onDash;
        }
      }

      e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
    return;
  }

  // Caso linee più spesse (thickness >= 2)
  // Calcola la direzione perpendicolare per lo spessore
  int perpX, perpY;

  // Determina la direzione dominante per il calcolo della perpendicolare
  if (dx >= abs(dy)) {
    // Linea più orizzontale - perpendicolare verticale
    perpX = 0;
    perpY = 1;
  } else {
    // Linea più verticale - perpendicolare orizzontale
    perpX = 1;
    perpY = 0;
  }

  int halfThickness = thickness / 2;
  int thicknessOffset = (thickness % 2 == 0) ? 1 : 0;  // Compensazione per spessori pari

  while (1) {
    if (onDash) {
      // Disegna tutti i pixel dello spessore richiesto
      for (int i = -halfThickness; i < halfThickness + thicknessOffset; i++) {
        int px = x0 + i * perpX;
        int py = y0 + i * perpY;
        setPixel(px, py);
      }
    }

    if (x0 == x1 && y0 == y1) break;

    if (lenDash) {
      count++;
      if (count == lenDash) {
        count = 0;
        onDash = !onDash;
      }
    }

    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}



// Disegna un cerchio con algoritmo di Bresenham
void drawDisp::drawCircle(int16_t cx, int16_t cy, int16_t r) {
  int x = r;
  int y = 0;
  int err = 0;
  while (x >= y) {
    setPixel(cx + x, cy + y);
    setPixel(cx + y, cy + x);
    setPixel(cx - y, cy + x);
    setPixel(cx - x, cy + y);
    setPixel(cx - x, cy - y);
    setPixel(cx - y, cy - x);
    setPixel(cx + y, cy - x);
    setPixel(cx + x, cy - y);
    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x + 1);
    }
  }
}

void drawDisp::drawHorizontalLine(int16_t x1, int16_t x2, int16_t y, int16_t lenDash) {
  int16_t count = 0;
  bool onDash = true;
  if (y<0 || y>=ly[pBuff]) return;
  for (int16_t x = x1; x <= x2; x++) {
    if (onDash) setPixel(x, y);
    if (lenDash) {
      count++;
      if (count == lenDash) {
        count = 0;
        onDash = !onDash;
      }
    }
  }
}
void drawDisp::drawVerticalLine(int16_t x, int16_t y1, int16_t y2, int16_t lenDash) {
  int16_t count = 0;
  bool onDash = true;
  if (x<0 || x>=lx[pBuff]) return;
  for (int16_t y = y1; y <= y2; y++) {
    if (onDash) setPixel(x, y);
    if (lenDash) {
      count++;
      if (count == lenDash) {
        count = 0;
        onDash = !onDash;
      }
    }
  }
}

void drawDisp::drawFilledCircle(int16_t x0, int16_t y0, int16_t r) {
  int x = r;
  int y = 0;
  int err = 0;

  while (x >= y) {
    // Disegna le linee orizzontali per tutti gli ottanti
    drawHorizontalLine(x0 - x, x0 + x, y0 + y);
    drawHorizontalLine(x0 - x, x0 + x, y0 - y);
    drawHorizontalLine(x0 - y, x0 + y, y0 + x);
    drawHorizontalLine(x0 - y, x0 + y, y0 - x);

    y++;
    err += 1 + 2 * y;
    if (2 * (err - x) + 1 > 0) {
      x--;
      err += 1 - 2 * x;
    }
  }
}

void drawDisp::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  int minx = fmax(0, fmin(x0, fmin(x1, x2)));
  int maxx = fmin(lx[pBuff] - 1, fmax(x0, fmax(x1, x2)));
  int miny = fmax(0, fmin(y0, fmin(y1, y2)));
  int maxy = fmin(ly[pBuff] - 1, fmax(y0, fmax(y1, y2)));
  int denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      int w0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2));
      int w1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2));
      int w2 = denom - w0 - w1;
      if ((denom > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) || (denom < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) setPixel(x, y);
    }
  }
}

// Lancetta triangolare (ore/minuti)
// angle: angolo in radianti da orizzontale destra, in senso antiorario, con 0 orrizondale dx
int drawDisp::drawTriangleHand(int16_t cx, int16_t cy, double angle,
                               int16_t length, int16_t base_width, int16_t offset, int32_t color) {
  // verteici: punta, base sinistra, base destra
  int ret = pBuff;
  double dir_x = cos(angle);
  double dir_y = sin(angle);
  // punta
  int px = cx + (int)round(length * dir_x);
  int py = cy + (int)round(length * dir_y);
  // centro di rotazione spostato di 'offset' lungo la direzione
  int rcx = cx + (int)round(offset * dir_x);
  int rcy = cy + (int)round(offset * dir_y);
  // vettore perpendicolare
  double perp_x = -dir_y;
  double perp_y = dir_x;
  // metà larghezza base
  int half = base_width / 2;
  int bx1 = rcx + (int)round(half * perp_x);
  int by1 = rcy + (int)round(half * perp_y);
  int bx2 = rcx - (int)round(half * perp_x);
  int by2 = rcy - (int)round(half * perp_y);
  //drawTriangle(px, py, bx1, by1, bx2, by2);
  int mmx = min(px, min(bx1, bx2));
  int mxx = max(px, max(bx1, bx2));
  int mmy = min(py, min(by1, by2));
  int mxy = max(py, max(by1, by2));
  //Serial.printf("%d %d %d %d \n",mmx,mxx,mmy,mxy);
  if (color >= 0) ret = creatCanvas(mmx, mmy, mxx, mxy, color);
  fillTriangle(px, py, bx1, by1, bx2, by2);
  return ret;
}

int drawDisp::creatCanvas(uint16_t _lx, uint16_t _ly) {
  return creatCanvas(0, 0, _lx, _ly, color[0]);
}

int drawDisp::clearCanvas(void) {
  int ret = -1;
  if (buff[pBuff]) {
    memset(buff[pBuff], 0, stride[pBuff] * ly[pBuff]);
    ret = pBuff;
  }
  return ret;
}

int drawDisp::creatCanvas(uint16_t _ox, uint16_t _oy, uint16_t _lx, uint16_t _ly, uint16_t _color) {
  int ret = -1;
  if (nBuff < MAXCANVAS) {
    nBuff++;
    pBuff = nBuff - 1;
    if (buff[pBuff]) delete buff[pBuff];
    lx[pBuff] = _lx;
    ly[pBuff] = _ly;
    ox[pBuff] = _ox;
    oy[pBuff] = _oy;
    stride[pBuff] = (lx[pBuff] + 7) >> 3;
    color[pBuff] = _color;
    buff[pBuff] = new uint8_t[stride[pBuff] * ly[pBuff]];
    if ((ret = clearCanvas()) != -1) priority[pBuff] = pBuff;
  }
  return ret;
}

void drawDisp::axis(uint16_t lx, uint16_t ly, uint16_t nxTick, uint16_t nyTick, uint16_t lenTick, uint16_t gridDash) {
  uint16_t x1 = lx - 1, y1 = ly - 1;
  uint16_t xn = lx / nxTick;
  uint16_t yn = ly / nyTick;
  creatCanvas(lx, ly);
  drawLine(0, y1, x1, y1);
  drawLine(0, 0, 0, y1);
  if (xn > 0)
    for (uint16_t i = xn; i < lx; i += xn) drawLine(i, y1, i, y1 - lenTick);
  if (yn > 0)
    for (uint16_t i = yn; i < ly; i += yn) drawLine(0, ly - i, lenTick, ly - i);
  if (gridDash) {
    drawHorizontalLine(0, lx, ly / 2, gridDash);
    drawVerticalLine(lx / 2, 0, ly, gridDash);
  }
}

void drawDisp::plot(uint16_t nPoint, double *xVal, float *yVal, double xMinScale, double xMaxScale, float yMinScale, float yMaxScale, uint16_t color, int16_t thickness) {
  if (lx[0] && ly[0]) {
    creatCanvas(0, 0, lx[0], ly[0], color);
    for (uint16_t i = 0; i < nPoint; i++) {
      if (i > 0) {
        uint16_t vx0 = dScale(xVal[i - 1], 0, lx[0], xMinScale, xMaxScale);
        uint16_t vx1 = dScale(xVal[i], 0, lx[0], xMinScale, xMaxScale);
        uint16_t vy0 = ly[0] - dScale(yVal[i - 1], 0, ly[0], yMinScale, yMaxScale);
        uint16_t vy1 = ly[0] - dScale(yVal[i], 0, ly[0], yMinScale, yMaxScale);
        drawLine(vx0, vy0, vx1, vy1, thickness);
      }
    }
  }
}

int16_t drawDisp::dScale(double x, double minScale, double maxScale, double minValue, double maxValue) {
  x = (x > maxValue ? maxValue : x);
  x = (x < minValue ? minValue : x);
  return (uint16_t)((x - minValue) * (maxScale - minScale) / (maxValue - minValue) + minScale);
}

void drawDisp::drawArcCircle(int cx, int cy, int radius, double a1_deg, double a2_deg) {
  // normalize angles to [0,360)
  a1_deg -= 90.;
  a2_deg -= 90.;
  a1_deg = fmod(fmod(a1_deg, 360.0) + 360.0, 360.0);
  a2_deg = fmod(fmod(a2_deg, 360.0) + 360.0, 360.0);

  int x = radius;
  int y = 0;
  int err = 0;
  while (x >= y) {
    // eight symmetric points
    struct Pt {
      int px, py;
    } pts[8] = {
      { cx + x, cy + y }, { cx + y, cy + x }, { cx - y, cy + x }, { cx - x, cy + y }, 
      { cx - x, cy - y }, { cx - y, cy - x }, { cx + y, cy - x }, { cx + x, cy - y }
    };
    for (int i = 0; i < 8; i++) {
      int px = pts[i].px;
      int py = pts[i].py;
      // compute angle of this point relative to center
      double ang = atan2(py - cy, px - cx) * 180.0 / M_PI;
      if (ang < 0) ang += 360.0;
      bool inArc;
      if (a1_deg <= a2_deg) inArc = (ang >= a1_deg && ang <= a2_deg);
      else inArc = (ang >= a1_deg || ang <= a2_deg);
      if (inArc) setPixel(px, py);
    }
    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x) + 1;
    }
  }
}

void drawDisp::drawCircleLine(uint16_t cx, uint16_t cy, uint16_t radius, double angdeg, uint16_t lenLine) {
  //angdeg += 90;
  double x0 = cos(angdeg * M_PI / 180.) * (radius - lenLine) + cx;
  double y0 = sin(angdeg * M_PI / 180.) * (radius - lenLine) + cy;
  double x1 = cos(angdeg * M_PI / 180.) * (radius) + cx;
  double y1 = sin(angdeg * M_PI / 180.) * (radius) + cy;
  y0 = ly[pBuff] - y0;
  y1 = ly[pBuff] - y1;
  //Serial.printf("%d %d %d %d %d\n", (int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1,ly[pBuff]);

  drawLine((int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1);
}

void drawDisp::drawCircleLabel(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t dist, char *Label[], uint16_t nLabel, uint16_t startLabel) {

  uint16_t lw = 0, lh = 0;
  double st = 12.0 / nLabel;
  for (int j = 0; j < nLabel; j++) {
    if (j < startLabel && j != 0) continue;
    boxString(Label[j], lw, lh);
    double angle = (2.0 * M_PI * (st * j)) / 12.0 - M_PI / 2;  // 3h=90°,6h=180°,etc.
    //double ac=cos(angle));
    //    Serial.printf("%d %f %f %f\n",j,angle,cos(angle),sin(angle));
    int tx = -lw / 2 + cx + (int)round((radius - dist - 4 - lw / 2) * cos(angle));
    int ty = -lh / 2 + cy + (int)round((radius - dist - 4 - lh / 2) * sin(angle));
    drawString(tx, ty, Label[j]);
  }
}

void drawDisp::drawCircleTick(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t long_tick, uint16_t short_tick, uint16_t startTick) {
  for (int i = startTick; i < 60; i++) {
    double angle = ((2.0 * M_PI) * i) / 60.0 - M_PI / 2;
    int x_outer = cx + (int)round(radius * cos(angle));
    int y_outer = cy + (int)round(radius * sin(angle));
    int tick_len = (i % 5 == 0) ? long_tick : short_tick;
    int x_inner = cx + (int)round((radius - tick_len) * cos(angle));
    int y_inner = cy + (int)round((radius - tick_len) * sin(angle));
    drawLine(x_inner, y_inner, x_outer, y_outer);
  }
}



void drawDisp::drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  drawLine(x0, y0, x1, y1);
  drawLine(x1, y1, x2, y2);
  drawLine(x2, y2, x0, y0);
}

bool drawDisp::isCharReduction(char c) {
  if (charReduction) {
    const char *p = charReduction;
    while (*p) {
      if (*p == c) return true;
      p++;
    }
  }
  return false;
}

void drawDisp::boxString(char *s, uint16_t &lw, uint16_t &lh, uint16_t Direction) {
  int l = strlen(s);
  lw = 0;
  //if (Direction == WRITE_HOR) lh = lyfont;
  //else lh = lyfont * l;
  if (Direction == WRITE_HOR) lh = yAdvance;
  else lh = yAdvance * l;
  for (int n = 0; n < l; n++) {
    uint8_t c = *(s + n) - (uint8_t)pgm_read_byte(&gfxFont->first);
    GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
    int16_t r = pgm_read_byte(&glyph->xAdvance);
    if (isCharReduction(*(s + n))) r = (int)floor((double)r / 2.);
    if (Direction == WRITE_HOR) lw += r;
    else lw = (lw < r ? r : lw);
  }
}


int drawDisp::creatScreen(uint16_t _xStart, uint16_t _yStart, int16_t _lx, uint16_t _ly) {
  xStart = _xStart;
  yStart = _yStart;
  xposLine = yposLine = 0;
  int ret = creatCanvas(_lx, _ly);
  if (ret >= 0) refreshMem(xStart, yStart, 0, 0, lx[pBuff], ly[pBuff], false);
  return ret;
}

void drawDisp::printLine(char *s) {
  drawString(xposLine, yposLine, s, true);
}

int16_t drawDisp::nextline(void) {
  yposLine += yAdvance;
  xposLine = 0;
  if (yposLine >= ly[pBuff]) {
    yposLine -= yAdvance;
    rowUp(yAdvance + yposLine % yAdvance);
    refreshMem(xStart, yStart, 0, 0, lx[pBuff], ly[pBuff], false);
  }
  return yposLine;
}



int drawDisp::drawString(int16_t x0, int16_t y0, char *s, bool onScreen, uint16_t Direction) {
  int ret = 0;
  int l = strlen(s);
  uint8_t c;
  GFXglyph *glyph;
  int16_t xStep = 0, yStep = 0;
  uint8_t w, h, xx, yy, bits = 0, bit = 0;
  int8_t xo, xa, yo;
  uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);
  if (onScreen) Direction = WRITE_HOR;
  if (buff[pBuff] == NULL) {
    lx[pBuff] = 0;
    if (Direction == WRITE_HOR) ly[pBuff] = lyfont;
    else ly[pBuff] = lyfont * l;
    for (int n = 0; n < l; n++) {
      c = *(s + n) - (uint8_t)pgm_read_byte(&gfxFont->first);
      glyph = pgm_read_glyph_ptr(gfxFont, c);
      //w = pgm_read_byte(&glyph->width);
      //xo = pgm_read_byte(&glyph->xOffset);
      xa = pgm_read_byte(&glyph->xAdvance);
      if (isCharReduction(*(s + n))) xa = (int)floor((double)xa / 2.);
      if (Direction == WRITE_HOR) lx[pBuff] += xa;
      else lx[pBuff] = (lx[pBuff] < xa ? xa : lx[pBuff]);
      //yo = pgm_read_byte(&glyph->yOffset);
    }
    //Serial.printf("lx=%d ly=%d\n", lx, ly);
    creatCanvas(lx[pBuff], ly[pBuff]);
  }
  if (buff[pBuff]) {
    for (int n = 0; n < l; n++) {
      bits = 0;
      bit = 0;
      uint8_t fc = (uint8_t)pgm_read_byte(&gfxFont->first);
      uint8_t lc = (uint8_t)pgm_read_byte(&gfxFont->last);
      c = *(s + n);
      if (c < fc || c > lc) {
        if (c == '\n' && onScreen) {
          y0 = nextline();
          x0 = 0;
        }
        continue;
      }
      c -= fc;
      glyph = pgm_read_glyph_ptr(gfxFont, c);
      w = pgm_read_byte(&glyph->width);
      h = pgm_read_byte(&glyph->height);
      xo = pgm_read_byte(&glyph->xOffset);
      xa = pgm_read_byte(&glyph->xAdvance);
      yo = pgm_read_byte(&glyph->yOffset);
      if (isCharReduction(*(s + n))) {
        xa = (int)floor((double)xa / 2.);
        xo = (int)floor((double)xo / 2.);
      }
      uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
      for (yy = 0; yy < h; yy++) {
        for (xx = 0; xx < w; xx++) {
          if (!(bit++ & 7)) {
            bits = pgm_read_byte(&bitmap[bo++]);
          }
          if (bits & 0x80) {
            setPixel(xStep + x0 + xo + xx, yStep + baseline + yo + yy + y0);
            if (onScreen) disp->drawPixel(xStart + xStep + x0 + xo + xx, yStart + yStep + baseline + yo + yy + y0, color[pBuff]);
            //writePixel(x + xo + xx, y + yo + yy, color);
          }
          bits <<= 1;
        }
      }
      if (Direction == WRITE_HOR) xStep += xa;
      else yStep += lyfont;
      if (onScreen) {
        xposLine += xa;
        if (xposLine >= lx[pBuff]) nextline();
      }
    }

  } else ret = -1;
  return ret;
}

int drawDisp::drawString(char *s) {
  return drawString(0, 0, s);
}

int drawDisp::refresh(int16_t x0, int16_t y0, bool onXor) {
  return refresh(x0, y0, 0, 0, lx[0], ly[0], onXor);
}

int drawDisp::drawPrint(int16_t x0, int16_t y0, char *s) {
  drawString(s);
  return refresh(x0, y0);
}

int drawDisp::refreshMem(int16_t x0, int16_t y0, int16_t xb, int16_t yb, int16_t lxb, int16_t lyb, bool onXor) {
  int ret = 0;
  //Serial.printf("%d %d %d %d %d %d\n", x0, y0, xb, yb, lxb, lyb);
  if (buff[0]) {
    uint32_t idx;
    int8_t mask;
    uint16_t ac, mc;
    disp->setAddrWindow(x0 + xb, y0 + yb, x0 + xb + lxb - 1, y0 + yb + lyb - 1);
    disp->setCS(LOW);
    //digitalWrite(pinDC, HIGH);
    for (uint16_t j = yb; j < lyb + yb; j++) {
      for (uint16_t i = xb; i < lxb + xb; i++) {
        ac = bkcolor;
        mc = 0;
        for (int pp = 0; pp < MAXCANVAS; pp++) {
          int p = priority[pp];
          if (p < 0 || p >= MAXCANVAS) continue;
          if (buff[p]) {
            int ii = i - ox[p], jj = j - oy[p];
            if (ii >= 0 && ii < lx[p] && jj >= 0 && jj < ly[p]) {
              idx = jj * stride[p] + (ii >> 3);
              mask = 0x80 >> (ii & 7);
              if (buff[p][idx] & mask) {
                if (!onXor) ac = color[p];
                else if (mc) ac ^= color[p];
                else mc = ac = color[p];
              }
            }
          }
        }
        //idx = j * stride[0] + (i >> 3);
        //mask = 0x80 >> (i & 7);
        //if (buff[0][idx] & mask) SPI.write16(color[0]); else SPI.write16(bkcolor);
        //disp->spi->write16(ac);
        disp->writeData16(ac);
        if (((uint32_t)j * (uint32_t)i) % 1000 == 0) yield();
      }
    }
  } else ret = -1;
  disp->setCS(HIGH);
  return ret;
}

int drawDisp::refresh(int16_t x0, int16_t y0, int16_t xb, int16_t yb, int16_t lxb, int16_t lyb, bool onXor) {
  int ret = refreshMem(x0, y0, xb, yb, lxb, lyb, onXor);
  for (int i = 0; i < MAXCANVAS; i++) {
    delete buff[i];
    buff[i] = NULL;
  }
  clrbuff();
  return ret;
}

int drawDisp::setCanvas(int16_t nCanvas) {
  int ret = -1;
  if (nCanvas >= 0 && nCanvas < nBuff) ret = pBuff = nCanvas;
  return ret;
}


int drawDisp::setPriority(int16_t nCanvas, int16_t nPriority) {
  int ret = -1;
  if (nPriority >= 0 && nPriority < nBuff && nCanvas >= 0 && nCanvas < nBuff) {
    int p = -1;
    for (int i = 0; i < nBuff; i++)
      if (priority[i] == nCanvas) {
        p = i;
        break;
      }
    if (p >= 0) {
      int8_t sw = priority[p];
      priority[p] = nPriority;
      priority[nPriority] = sw;
      ret = 0;
    }
  }
  return ret;
}

int drawDisp::rowUp(uint16_t nPixels) {
  int ret = 0;
  if (buff[pBuff]) {
    uint32_t ln = nPixels * stride[pBuff];
    uint32_t lb = stride[pBuff] * ly[pBuff];
    uint32_t i;
    if (ln < lb) {
      for (i = 0; i < lb - ln; i++) buff[pBuff][i] = buff[pBuff][i + ln];
      for (i = lb - ln; i < lb; i++) buff[pBuff][i] = 0;
    }
  }
  return 0;
}