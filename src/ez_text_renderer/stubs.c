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

#define CAML_NAME_SPACE
#include "caml/mlvalues.h"
#include "caml/memory.h"
#include "caml/alloc.h"
#include "caml/fail.h"

#include "text_renderer.h"

static void
check_error(
  int result)
{
  switch (result) {
    case TR_OK:
      break;
    case TR_GENERIC_ERROR:
      caml_failwith("Generic error");
      break;
    case TR_OUT_OF_MEMORY:
      caml_raise_out_of_memory();
      break;
    case TR_INITIALIZATION_FAILED:
      caml_failwith("Library initialization failed");
      break;
    case TR_NOT_INITIALIZED:
      caml_failwith("Library not initialized");
      break;
    case TR_UNSUPPORTED_FONT_FORMAT :
      caml_failwith("Unsupported font format");
      break;
    case TR_ERROR_OPENING_FONT:
      caml_failwith("Error opening font");
      break;
    case TR_FONT_NOT_UNICODE:
      caml_failwith("Font does not support Unicode");
      break;
    case TR_UNABLE_TO_SET_SIZE:
      caml_failwith("Unable to set specified font size");
      break;
    case TR_FONT_NOT_SET:
      caml_failwith("The font must be set before calling this function");
      break;
    case TR_CANT_OPEN_FILE:
      caml_failwith("Unable to open file");
      break;
    default:
      caml_failwith("Unknown error");
      break;
  }
}

static bool
int_in_range(
  int i,
  int min,
  int max)
{
  if (max < min) {
    int t = max;
    max = min;
    min = t;
  }
  return ((i >= min) && (i <= max));
}

static uint32_t
Color_val(
  value mlColor)
{
  CAMLparam1(mlColor);
  int b = Int_val(Field(mlColor, 0));
  int g = Int_val(Field(mlColor, 1));
  int r = Int_val(Field(mlColor, 2));
  int a = Int_val(Field(mlColor, 3));
  if (!int_in_range(a, 0, 0xFF))
    caml_invalid_argument("Alpha component not in range 0-255");
  if (!int_in_range(r, 0, 0xFF))
    caml_invalid_argument("Red component not in range 0-255");
  if (!int_in_range(g, 0, 0xFF))
    caml_invalid_argument("Green component not in range 0-255");
  if (!int_in_range(b, 0, 0xFF))
    caml_invalid_argument("Blue component not in range 0-255");
  uint32_t color = argb_to_int(a, r, g, b);
  CAMLreturnT(uint32_t, color);
}

CAMLprim value
ml_tr_init(
  value mlUnit)
{
  CAMLparam1(mlUnit);
  int result = tr_init();
  check_error(result);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_tr_release(
  value mlUnit)
{
  CAMLparam1(mlUnit);
  int result = tr_release();
  check_error(result);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_tr_set_font(
  value mlFontFile,
  value mlFontHeight)
{
  CAMLparam2(mlFontFile, mlFontHeight);
  const char *font_file = String_val(mlFontFile);
  int height = Int_val(mlFontHeight);
  int result = tr_set_font(font_file, height);
  check_error(result);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_tr_compute_text_width(
  value mlText)
{
  CAMLparam1(mlText);
  const char *text = String_val(mlText);
  int width = 0;
  int result = tr_compute_text_width(text, &width);
  check_error(result);
  CAMLreturn(Val_int(width));
}

CAMLprim value
ml_tr_render_text(
  value mlText,
  value mlFrontColor,
  value mlBackColor,
  value mlOutArea,
  value mlOutArray,
  value mlOutDimensions)
{
  CAMLparam5(mlText, mlFrontColor, mlBackColor, mlOutArea, mlOutArray);
  CAMLxparam1(mlOutDimensions);
  CAMLlocal1(mlColor);

  const char *text = String_val(mlText);
  uint32_t front_color = Color_val(mlFrontColor);
  uint32_t back_color = Color_val(mlBackColor);
  int x = Int_val(Field(mlOutArea, 0));
  int y = Int_val(Field(mlOutArea, 1));
  int width = Int_val(Field(mlOutArea, 2));
  int height = Int_val(Field(mlOutArea, 3));
  int dest_width = Int_val(Field(mlOutDimensions, 0));
  int dest_height = Int_val(Field(mlOutDimensions, 1));

  int data_size = width * height * BYTES_PER_PIXEL;
  uint8_t *data = malloc(data_size * sizeof(uint8_t));
  if (data == NULL) {
    caml_raise_out_of_memory();
  }

  int result =
    tr_render_text(text, front_color, back_color,
                   0, 0, width, height, data, width, height);
  if (result != TR_OK) {
    free(data);
    check_error(result);
  }

  for (int i = 0; (i < height) && (y + i < dest_height); ++i) {
    for (int j = 0; (j < width) && (x + j < dest_width); ++j) {
      int src_idx = (i * width + j) * BYTES_PER_PIXEL;
      int dst_idx = (y + i) * dest_width + (x + j);
      mlColor = caml_alloc_tuple(4);
      Store_field(mlColor, 0, Val_int(data[src_idx + 0])); // blue
      Store_field(mlColor, 1, Val_int(data[src_idx + 1])); // green
      Store_field(mlColor, 2, Val_int(data[src_idx + 2])); // red
      Store_field(mlColor, 3, Val_int(data[src_idx + 3])); // alpha
      Store_field(mlOutArray, dst_idx, mlColor);
    }
  }

  free(data);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_tr_render_text_bc(value *a, int n)
{
  return ml_tr_render_text(a[0],a[1],a[2],a[3],a[4],a[5]);
}

CAMLprim value
ml_tr_dump_image(
  value mlArray,
  value mlDimensions,
  value mlOutFile)
{
  CAMLparam3(mlArray, mlDimensions, mlOutFile);
  CAMLlocal1(mlColor);

  int width = Int_val(Field(mlDimensions, 0));
  int height = Int_val(Field(mlDimensions, 1));
  const char *outfile = String_val(mlOutFile);

  int data_size = width * height * BYTES_PER_PIXEL;
  uint8_t *data = malloc(data_size * sizeof(uint8_t));
  if (data == NULL) {
    caml_raise_out_of_memory();
  }

  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      int src_idx = i * width + j;
      int dst_idx = src_idx * BYTES_PER_PIXEL;
      mlColor = Field(mlArray, src_idx);
      data[dst_idx + 0] = Int_val(Field(mlColor, 0)); // blue
      data[dst_idx + 1] = Int_val(Field(mlColor, 1)); // green
      data[dst_idx + 2] = Int_val(Field(mlColor, 2)); // red
      data[dst_idx + 3] = Int_val(Field(mlColor, 3)); // alpha
    }
  }

  int result = tr_dump_image(data, width, height, outfile);
  if (result != TR_OK) {
    free(data);
    check_error(result);
  }

  free(data);
  CAMLreturn(Val_unit);
}
