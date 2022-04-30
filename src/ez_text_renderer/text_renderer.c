/**************************************************************************/
/*                                                                        */
/*  Copyright (c) 2021 OCamlPro                                           */
/*                                                                        */
/*  All rights reserved.                                                  */
/*  This file is distributed under the terms of the GNU Lesser General    */
/*  Public License version 2.1, with the special exception on linking     */
/*  described in the LICENSE.md file in the root directory.               */
/*                                                                        */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "text_renderer.h"

#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))

#define alpha_blend(alpha,src,dst) \
  (((alpha) * (src) + (0xFF - (alpha)) * (dst)) / 0xFF)

static bool initialized = false;
static bool font_set = false;
static FT_Library library;
static FT_Face face;

static wchar_t *
mb_to_wc(
  const char *mb_text)
{
  size_t len = mbstowcs(NULL, mb_text, 0) + 1;
  wchar_t *wc_text = malloc(len * sizeof(wchar_t));
  if (wc_text != NULL) {
    mbstowcs(wc_text, mb_text, len);
  }
  return wc_text;
}

static void
tr_alpha_blend(
  uint32_t color,
  uint8_t *dest)
{
  uint8_t a = alpha_of_int(color);
  uint8_t r = red_of_int(color);
  uint8_t g = green_of_int(color);
  uint8_t b = blue_of_int(color);
  uint8_t alpha = a + dest[3] * (0xFF - a) / 0xFF;
  dest[0] = (b * a + dest[0] * dest[3] * (0xFF - a) / 0xFF) / alpha;
  dest[1] = (g * a + dest[1] * dest[3] * (0xFF - a) / 0xFF) / alpha;
  dest[2] = (r * a + dest[2] * dest[3] * (0xFF - a) / 0xFF) / alpha;
  dest[3] = alpha;
}

int
tr_init()
{
  int error;

  if (initialized) {
    return TR_OK;
  }

  setlocale(LC_ALL, "");

  error = FT_Init_FreeType(&library);
  if (error) {
    return TR_INITIALIZATION_FAILED;
  }

  initialized = true;

  return TR_OK;
}

int
tr_release()
{
  if (!initialized) {
    return TR_OK;
  }

  FT_Done_FreeType(library);

  initialized = false;

  return TR_OK;
}

int
tr_set_font(
  const char *font_file,
  int height)
{
  int error;

  if (!initialized) {
    return TR_NOT_INITIALIZED;
  }

  error = FT_New_Face(library, font_file, 0 /* face_index */, &face);
  if (error == FT_Err_Unknown_File_Format) {
    return TR_UNSUPPORTED_FONT_FORMAT;
  }
  else if (error) {
    return TR_ERROR_OPENING_FONT;
  }

  error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
  if (error) {
    return TR_FONT_NOT_UNICODE;
  }

  int try_height = height;
  do {
    FT_Set_Pixel_Sizes(face, 0, try_height--);
    if (error) {
      return TR_UNABLE_TO_SET_SIZE;
    }
  } while ((height <
	    (face->size->metrics.ascender +
	     labs(face->size->metrics.descender)) >> 6) ||
           (height <
	    (face->size->metrics.height) >> 6));

  font_set = true;

/*
  FT_Size_RequestRec req = { FT_SIZE_REQUEST_TYPE_BBOX, 0, height * 64,0, 0 };
  FT_Request_Size(face, &req);
*/
/*
  printf("num_faces: %ld\n", face->num_faces);
  printf("face_index: %ld\n", face->face_index);
  printf("face_flags: %ld\n", face->face_flags);
  printf("num_glyphs: %ld\n", face->num_glyphs);
  printf("family_name: %s\n", face->family_name);
  printf("style_name: %s\n", face->style_name);
  printf("num_fixed_sizes: %d\n", face->num_fixed_sizes);
  printf("num_charmaps: %d\n", face->num_charmaps);

  printf("bbox.xMin: %ld\n", face->bbox.xMin >> 6);
  printf("bbox.yMin: %ld\n", face->bbox.yMin >> 6);
  printf("bbox.xMax: %ld\n", face->bbox.xMax >> 6);
  printf("bbox.yMax: %ld\n", face->bbox.yMax >> 6);
  printf("units_per_EM: %d\n", face->units_per_EM >> 6);
  printf("ascender: %d\n", face->ascender >> 6);
  printf("descender: %d\n", face->descender >> 6);
  printf("height: %d\n", face->height >> 6);

  printf("metrics.ascender : %ld\n", face->size->metrics.ascender >> 6);
  printf("metrics.descender : %ld\n", face->size->metrics.descender >> 6);
  printf("metrics.height : %ld\n", face->size->metrics.height >> 6);
*/
  return TR_OK;
}

int
tr_compute_text_width(
  const char *mb_text,
  int *out_width)
{
  if (!initialized) {
    return TR_NOT_INITIALIZED;
  }

  if (!font_set) {
    return TR_FONT_NOT_SET;
  }

  wchar_t *wc_text = mb_to_wc(mb_text);
  if (wc_text == NULL) {
    return TR_OUT_OF_MEMORY;
  }

  size_t len = wcslen(wc_text);

  int width = 0;
  for (size_t i = 0; i < len; ++i) {

    int error = FT_Load_Char(face, wc_text[i], FT_LOAD_DEFAULT);
    if (error) {
      continue; // we just ignore character errors
    }

    width += face->glyph->advance.x >> 6; // should adjust last character
  }

  *out_width = width;

  free(wc_text);

  return TR_OK;
}

static void
tr_draw_glyph(
  FT_GlyphSlot glyph,
  uint32_t front_color,
  uint32_t back_color,
  int pen_x,
  int pen_y,
  int w,
  int h,
  uint8_t *dest)
{
  uint8_t fa = alpha_of_int(front_color);
  uint8_t fr = red_of_int(front_color);
  uint8_t fg = green_of_int(front_color);
  uint8_t fb = blue_of_int(front_color);

  uint8_t ba = alpha_of_int(back_color);
  uint8_t br = red_of_int(back_color);
  uint8_t bg = green_of_int(back_color);
  uint8_t bb = blue_of_int(back_color);

  for (int i = 0; i < glyph->bitmap.rows; ++i) {

    int di = pen_y - glyph->bitmap_top + i;
    if (di < 0) continue;
    else if (di > h) break;

    for (int j = 0; j < glyph->bitmap.width; ++j) {

      int dj = pen_x + glyph->bitmap_left + j;
      if (dj < 0) continue;
      else if (dj > w) break;

      int s_idx = i * glyph->bitmap.pitch + j;
      uint8_t alpha = glyph->bitmap.buffer[s_idx];

      int d_idx;

      switch (alpha) {

        case 0x00:
          // Fully transparent pixel: don't bother drawing
          break;

        case 0xFF:
          // Fully opaque pixel: no alpha computation
          d_idx = (di * w + dj) * BYTES_PER_PIXEL;
          dest[d_idx + 0] = fb;
          dest[d_idx + 1] = fg;
          dest[d_idx + 2] = fr;
          dest[d_idx + 3] = fa;
          break;

        default: {
          // Semi-transparent pixel: apply alpha
          uint8_t a = alpha_blend(alpha, fa, ba);
          uint8_t r = alpha_blend(alpha, fr, br);
          uint8_t g = alpha_blend(alpha, fg, bg);
          uint8_t b = alpha_blend(alpha, fb, bb);
          d_idx = (di * w + dj) * BYTES_PER_PIXEL;
          dest[d_idx + 0] = alpha_blend(alpha, b, dest[d_idx + 0]);
          dest[d_idx + 1] = alpha_blend(alpha, g, dest[d_idx + 1]);
          dest[d_idx + 2] = alpha_blend(alpha, r, dest[d_idx + 2]);
          dest[d_idx + 3] = alpha_blend(alpha, a, dest[d_idx + 3]);
        }
      }
    }
  }
}

int tr_render_text(
  const char *mb_text,
  uint32_t front_color,
  uint32_t back_color,
  int x,
  int y,
  int w,
  int h,
  uint8_t *data,
  int dw,
  int dh)
{
  if (!initialized) {
    return TR_NOT_INITIALIZED;
  }

  if (!font_set) {
    return TR_FONT_NOT_SET;
  }

  wchar_t *wc_text = mb_to_wc(mb_text);
  if (wc_text == NULL) {
    return TR_OUT_OF_MEMORY;
  }

  int data_size = w * h * BYTES_PER_PIXEL;
  uint8_t *temp_data = malloc(data_size * sizeof(uint8_t));
  if (temp_data == NULL) {
    free(wc_text);
    return TR_OUT_OF_MEMORY;
  }

  int a = alpha_of_int(back_color);
  int r = red_of_int(back_color);
  int g = green_of_int(back_color);
  int b = blue_of_int(back_color);

  for (int i = 0; i < data_size; i += 4) {
    temp_data[i + 0] = b;
    temp_data[i + 1] = g;
    temp_data[i + 2] = r;
    temp_data[i + 3] = a;
  }

  int pen_x = 0;
  int pen_y = face->size->metrics.ascender >> 6;

  size_t len = wcslen(wc_text);

  for (size_t i = 0; i < len; ++i) {

    int error = FT_Load_Char(face, wc_text[i], FT_LOAD_RENDER);
    if (error) {
      continue; // we just ignore character errors
    }

    // FT_PIXEL_MODE_MONO = might be used
    // FT_PIXEL_MODE_BGRA = rare, used by Google
    // FT_PIXEL_MODE_GRAY2/GRAY4 = even rarer
    // FT_PIXEL_MODE_LCD/LCD_V = ???
    if (face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
      fprintf(stderr, "Unsupported pixel mode %d\n",
              face->glyph->bitmap.pixel_mode);
      continue; // should try to support others
    }

    tr_draw_glyph(face->glyph, front_color, back_color,
                  pen_x, pen_y, w, h, temp_data);

    pen_x += face->glyph->advance.x >> 6;
  }

  free(wc_text);

  for (int i = 0; (i < h) && (y + i < dh); ++i) {
    for (int j = 0; (j < w) && (x + j < dw); ++j) {
      int src_idx = (i * w + j) * BYTES_PER_PIXEL;
      int dst_idx = ((y + i) * dw + (x + j)) * BYTES_PER_PIXEL;
      uint32_t color = argb_to_int(temp_data[src_idx + 3],
                                   temp_data[src_idx + 2],
                                   temp_data[src_idx + 1],
                                   temp_data[src_idx + 0]);
      tr_alpha_blend(color, data + dst_idx);
    }
  }

  free(temp_data);

  return TR_OK;
}

int
tr_dump_image(
  const uint8_t *data,
  int dw,
  int dh,
  const char *outfile)
{
  FILE *out = fopen(outfile, "wb");
  if (out == NULL) {
    return TR_CANT_OPEN_FILE;
  }

  fprintf(out, "P6\n%d %d\n255\n", dw, dh);

  for (int i = 0; i < dw * dh * BYTES_PER_PIXEL; i += BYTES_PER_PIXEL) {
    fputc(data[i + 2], out); // red
    fputc(data[i + 1], out); // green
    fputc(data[i + 0], out); // blue
  }

  fclose(out);

  return TR_OK;
}


