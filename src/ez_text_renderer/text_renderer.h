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

#include <stdint.h>

#define BYTES_PER_PIXEL 4

#define alpha_of_int(i) (((i) & 0xFF000000) >> 24)
#define red_of_int(i)   (((i) & 0x00FF0000) >> 16)
#define green_of_int(i) (((i) & 0x0000FF00) >>  8)
#define blue_of_int(i)  (((i) & 0x000000FF) >>  0)

#define rgb_to_int(r,g,b) \
  (((      0xFF) << 24) | \
   (((r) & 0xFF) << 16) | \
   (((g) & 0xFF) <<  8) | \
   (((b) & 0xFF) <<  0))

#define argb_to_int(a,r,g,b) \
  ((((a) & 0xFF) << 24) | \
   (((r) & 0xFF) << 16) | \
   (((g) & 0xFF) <<  8) | \
   (((b) & 0xFF) <<  0))

#define TR_OK                       0
#define TR_GENERIC_ERROR            1
#define TR_OUT_OF_MEMORY            2
#define TR_INITIALIZATION_FAILED    3
#define TR_NOT_INITIALIZED          4
#define TR_UNSUPPORTED_FONT_FORMAT  5
#define TR_ERROR_OPENING_FONT       6
#define TR_FONT_NOT_UNICODE         7
#define TR_UNABLE_TO_SET_SIZE       8
#define TR_FONT_NOT_SET             9
#define TR_CANT_OPEN_FILE           10

int tr_init();
int tr_release();
int tr_set_font(const char *font_file, int height);
int tr_compute_text_width(const char *mb_text, int *out_width);
int tr_render_text(const char *mb_text, uint32_t front_color, uint32_t back_color, int x, int y, int w, int h, uint8_t *data, int dw, int dh);
int tr_dump_image(const uint8_t *data, int dw, int dh, const char *outfile);
