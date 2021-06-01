module C = Configurator.V1

let () =
  C.main ~name:"freetype" (fun c ->
      let default : C.Pkg_config.package_conf =
        { libs = ["-lfreetype"]; cflags = ["-I"; "/usr/include/freetype2"] }
      in
      let conf =
        match C.Pkg_config.get c with
        | None -> default
        | Some pc ->
            match (C.Pkg_config.query pc ~package:"freetype2") with
            | None -> default
            | Some deps -> deps
      in
      C.Flags.write_sexp "c_flags.sexp" conf.cflags;
      C.Flags.write_sexp "c_library_flags.sexp" conf.libs)
