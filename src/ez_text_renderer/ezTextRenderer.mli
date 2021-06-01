(**************************************************************************)
(*                                                                        *)
(*  Copyright (c) 2021 OCamlPro                                           *)
(*                                                                        *)
(*  All rights reserved.                                                  *)
(*  This file is distributed under the terms of the GNU Lesser General    *)
(*  Public License version 2.1, with the special exception on linking     *)
(*  described in the LICENSE.md file in the root directory.               *)
(*                                                                        *)
(*                                                                        *)
(**************************************************************************)

(** EzTextRenderer library

    This modules allows to perform text rendering of (UTF-8 encoded)
    Unicode strings to standard OCaml arrays. It is based on the
    FreeType library.

    It is currently limited to horizontal left-to-right text layout.

    Error handling is pretty basic: most errors will simply be
    raised as a Failure exception with an explanatory string.
    Failed memory allocations will raise Out_of_memory.

    A typical usage is as follows:

{[
    open EzTextRenderer

    let black = { blue = 0x00; green = 0x00; red = 0x00; alpha = 0xFF }
    let grey = { blue = 0x80; green = 0x80; red = 0x80; alpha = 0xFF }
    let white = { blue = 0xFF; green = 0xFF; red = 0xFF; alpha = 0xFF }

    let () =
      init ();

      let txt_height = 64 in

      set_font "/usr/share/fonts/truetype/freefont/FreeSerif.ttf" txt_height;

      let text = "Hello World !" in

      let txt_width = compute_text_width text in

      let img_size = { width = txt_width + 16; height = txt_height + 16 } in

      let image = Array.make (img_size.height * img_size.width) grey in

      render_text text black white
        { x = 8; y = 8; width = txt_width; height = txt_height }
        image img_size;

      dump_image image img_size "img.ppm";

      release ()
]}
 *)

(** Representation of a color, where each component must be between 0 and 255 *)
type color = {
  blue : int;
  green : int;
  red : int;
  alpha : int;
}

(** A position in an image, with (x,y) located at the top-left *)
type position = {
  x : int;
  y : int;
}

(** An image's dimensions *)
type dimension = {
  width : int;
  height : int;
}

(** An area of an image, starting at (x,y) and of size (width,height) *)
type area = {
  x : int;
  y : int;
  width : int;
  height : int;
}


(** Initialize the EzTextRenderer library. Must be called at least
    once before calling any other function in this module. *)
val init : unit -> unit

(** Free the EzTextRenderer library. Call it once you're done using
    the library. *)
val release : unit -> unit

(** Set the current font, where the first argument is a path to a font
    file (eg. /usr/share/fonts/truetype/freefont/FreeSerif.ttf) and
    the second is the font height in pixels. *)
val set_font : string -> int -> unit

(** Return the width of the specified text in pixels, for the currently
    selected font. Fails if no font has been selected. *)
val compute_text_width : string -> int

(** Render the specified text using the first color as front color and
    the second as background color in the given area of the provided
    array, representing an image whose dimensions are given as the
    last argument. Fails if no font has been selected. *)
val render_text : string -> color -> color -> area -> color array -> dimension -> unit

(** Dump the provided array, representing an image whose dimensions are
    given by the second argument, in a PPM file whose name is given
    as the last argument. *)
val dump_image : color array -> dimension -> string -> unit
