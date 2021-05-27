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

type position = {
  x : int;
  y : int;
}

type dimension = {
  width : int;
  height : int;
}

type area = {
  x : int;
  y : int;
  width : int;
  height : int;
}

type color = {
  blue : int;
  green : int;
  red : int;
  alpha : int;
}

external init : unit -> unit = "ml_tr_init"

external release : unit -> unit = "ml_tr_release"

external set_font : string -> int -> unit = "ml_tr_set_font"

external compute_text_width : string -> int = "ml_tr_compute_text_width"

external render_text : string -> color -> color -> area -> color array -> dimension -> unit = "ml_tr_render_text_bc" "ml_tr_render_text"

external dump_image : color array -> dimension -> string -> unit = "ml_tr_dump_image"
