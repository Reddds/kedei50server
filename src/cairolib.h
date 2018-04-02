#ifndef CAIROLIB_H
#define CAIROLIB_H

int cairo_test (cairo_t *cr);
void cairo_line(cairo_t *cr, double stroke_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t r, uint8_t g, uint8_t b);
void control_label(cairo_t *cr, uint16_t x, uint16_t y, double size, char *text, uint8_t r, uint8_t g, uint8_t b);
#endif
