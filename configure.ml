let config_mk = "config.mk"
let config_h = "config.h"

(* Configure script *)
open Cmdliner

let info =
  let doc = "Configures a package" in
  Term.info "configure" ~version:"0.1" ~doc

let output_file filename lines =
  let oc = open_out filename in
  let lines = List.map (fun line -> line ^ "\n") lines in
  List.iter (output_string oc) lines;
  close_out oc

let cc verbose c_program =
  let c_file = Filename.temp_file "configure" ".c" in
  let o_file = c_file ^ ".o" in
  output_file c_file c_program;
  let found = Sys.command (Printf.sprintf "cc -Werror -c %s -o %s %s" c_file o_file (if verbose then "" else "2>/dev/null")) = 0 in
  if Sys.file_exists c_file then Sys.remove c_file;
  if Sys.file_exists o_file then Sys.remove o_file;
  found

let run verbose cmd =
  if verbose then Printf.printf "running: %s\n" cmd;
  let out_file = Filename.temp_file "configure" ".stdout" in
  let code = Sys.command (Printf.sprintf "%s > %s" cmd out_file) in
  let ic = open_in out_file in
  let result = ref [] in
  let lines =
    try
      while true do
        let line = input_line ic in
        result := line :: !result
      done;
      !result
    with End_of_file -> !result in
  close_in ic;
  if code <> 0
  then failwith (Printf.sprintf "%s: %d: %s" cmd code (String.concat " " (List.rev lines)))
  else List.rev lines

let find_header verbose name =
  let c_program = [
    Printf.sprintf "#include <%s>" name;
    "int main(int argc, const char *argv){";
    "return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for %s: %s\n" name (if found then "ok" else "missing");
  found

let find_define verbose name =
  let c_program = [
    "#include <xenctrl.h>";
    "int main(int argc, const char *argv){";
    Printf.sprintf "int i = %s;" name;
    "return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for %s: %s\n" name (if found then "ok" else "missing");
  found

let find_struct_member verbose structure member =
  let c_program = [
    "#include <stdlib.h>";
    "#include <libxl.h>";
    "#include <xenctrl.h>";
    "#include <xenguest.h>";
    Printf.sprintf "void test(%s *s) {" structure;
    Printf.sprintf "  int r = s->%s;\n" member;
    "}";
    "int main(int argc, const char *argv){";
    "  return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for %s.%s: %s\n" structure member (if found then "ok" else "missing");
  found

let find_xenlight_4_4 verbose = find_struct_member verbose "libxl_physinfo" "outstanding_pages"

(* Only xen-4.4 had this before it got removed *)
let find_xc_domain_save_generation_id verbose =
  let c_program = [
    "#include <stdlib.h>";
    "#include <xenctrl.h>";
    "#include <xenguest.h>";
    "int main(int argc, const char *argv){";
    "  int r = xc_domain_save(/* xc_interface *xch*/NULL, /*int io_fd*/0, /*uint32_t dom*/0, /*uint32_t max_iters*/0,";
    "              /*uint32_t max_factor*/0, /*uint32_t flags*/0,";
    "              /*struct save_callbacks* callbacks*/NULL, /*int hvm*/0);";
    "  return 0;";
    "}";
  ] in
  let missing = cc verbose c_program in
  Printf.printf "Looking for xc_domain_save generation_id: %s\n" (if missing then "missing" else "found");
  not missing

let find_xentoollog verbose =
  let c_program = [
    "int main(int argc, const char *argv){";
    "  return 0;";
    "}";
  ] in
  let c_file = Filename.temp_file "configure" ".c" in
  let exe_file = c_file ^ ".exe" in
  output_file c_file c_program;
  let found = Sys.command (Printf.sprintf "cc -Werror %s -lxentoollog -o %s %s" c_file exe_file (if verbose then "" else "2>/dev/null")) = 0 in
  if Sys.file_exists c_file then Sys.remove c_file;
  if Sys.file_exists exe_file then Sys.remove exe_file;
  Printf.printf "Looking for xentoollog: %s\n" (if found then "found" else "missing");
  found

let find_xen_4_5 verbose =
  let c_program = [
    "#include <stdlib.h>";
    "#include <xenctrl.h>";
    "#include <xenguest.h>";
    "int main(int argc, const char *argv){";
    "  int r = xc_vcpu_getaffinity(NULL, 0,";
    "                              0, NULL, NULL, 0);";
    "  return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for xen-4.5: %s\n" (if found then "ok" else "missing");
  found

let find_xen_4_6 verbose =
  let c_program = [
    "#include <stdlib.h>";
    "#include <xenctrl.h>";
    "#include <xenguest.h>";
    "int main(int argc, const char *argv){";
    "  int r = xc_assign_device(NULL, 0, 0, 0);";
    "  return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for xen-4.6: %s\n" (if found then "ok" else "missing");
  found

let check_cores_per_socket verbose =
  let c_program = [
    "#include <stdlib.h>";
    "#include <xenctrl.h>";
    "#include <xenguest.h>";
    "int main(int argc, const char *argv){";
    "  int r = xc_domain_set_cores_per_socket(0,0,0);";
    "  return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for xc_domain_set_cores_per_socket: %s\n" (if found then "ok" else "missing");
  found

let check_domain_create_has_config_param verbose =
  let c_program = [
    "#include <stdlib.h>";
    "#include <xenctrl.h>";
    "#include <xenguest.h>";
    "int main(int argc, const char *argv){";
    "  int r = xc_domain_create(0, 0, 0, 0, 0, 0);";
    "  return 0;";
    "}";
  ] in
  let found = cc verbose c_program in
  Printf.printf "Looking for config parameter on xc_domain_create: %s\n" (if found then "ok" else "missing");
  found

(* PVH param was in domain_create_info from 4.4.0 - 4.8.0 *)
let check_for_missing_pvh_param verbose =
  let c_program = [
    "#include <libxl.h>";
    "#include <libxl_utils.h>";
    "libxl_domain_create_info *domain_create_info_c;";
    "int main(int argc, char **argv) {";
    "libxl_defbool t=domain_create_info_c -> pvh;";
    "return 0;";
    "}";
  ] in
  let present = cc verbose c_program in
  Printf.printf "Looking for missing pvh param on domain_create_info_c: %s\n" (if present then "present" else "missing");
  not present

let check_arm_header verbose =
  let lines = run verbose "uname -m" in
  let arch = List.hd lines in
  let arm = String.length arch >= 3 && String.sub arch 0 3 = "arm" in
  if arm then begin
    let header = "/usr/include/xen/arch-arm/hvm/save.h" in
    if not(Sys.file_exists header) then begin
      Printf.printf "Creating empty header %s\n" header;
      ignore(run verbose (Printf.sprintf "mkdir -p %s" (Filename.dirname header)));
      ignore(run verbose (Printf.sprintf "touch %s" header))
    end
  end

let disable_xenguest =
  let doc = "Don't build any xenguest binary" in
  Arg.(value & flag & info ["disable-xenguest"] ~docv:"DISABLE_XENGUEST" ~doc)

let choose_xenlight xen_4_4 xen_4_5 xen_4_6 xen_4_7 xen_4_9 =
  if xen_4_9 then "4.9" else
  if xen_4_7 then "4.7" else
  if xen_4_6 then "4.5" else
  if xen_4_5 then "4.5" else
    "4.4"

let choose_xenguest disable_xenguest xen_4_4 xen_4_5 xen_4_6 xen_4_7 xen_4_9 =
  if xen_4_7 || xen_4_9 then None else
  if xen_4_6 then Some "4.6" else
  if xen_4_5 then Some "4.4" else
  if xen_4_4 then Some "4.4" else
    None


let configure verbose disable_xenguest =
  check_arm_header verbose;
  let xenctrl  = find_header verbose "xenctrl.h" in
  let xenlight_4_4 = find_xenlight_4_4 verbose in
  let xen_4_4  = xenlight_4_4 in
  let xen_4_5  = find_xen_4_5 verbose in
  let xen_4_6  = find_xen_4_6 verbose in
  let xen_4_7  = check_domain_create_has_config_param verbose in
  let xen_4_9  = check_for_missing_pvh_param verbose in
  let cores_per_socket = check_cores_per_socket verbose in
  let domain_create_has_config = xen_4_7 in
  let xc_domain_save_generation_id = find_xc_domain_save_generation_id verbose in
  let have_viridian = find_define verbose "HVM_PARAM_VIRIDIAN" in
  if not xenctrl then begin
    Printf.fprintf stderr "Failure: we can't build anything without xenctrl.h\n";
    exit 1;
  end;
  if not xenlight_4_4 then begin
    Printf.fprintf stderr "Failure: we can't build anything without libxl from Xen 4.4 or greater\n";
    exit 1;
  end;
  let xenlight_ver = choose_xenlight xen_4_4 xen_4_5 xen_4_6 xen_4_7 xen_4_9 in
  (try Unix.unlink "xenlight" with Unix.Unix_error(Unix.ENOENT, _, _) -> ());
  Unix.symlink ("xenlight-" ^ xenlight_ver) "xenlight";

  (try Unix.unlink "xentoollog" with Unix.Unix_error(Unix.ENOENT, _, _) -> ());
  Unix.symlink ("xentoollog-" ^ xenlight_ver) "xentoollog";

  let xenguest = choose_xenguest disable_xenguest xen_4_4 xen_4_5 xen_4_6 xen_4_7 xen_4_9 in

  let xentoollog = find_xentoollog verbose in

  (* Write config.mk *)
  let lines =
    [ "# Warning - this file is autogenerated by the configure script";
      "# Do not edit";
      Printf.sprintf "ENABLE_XENGUEST42=--%s-xenguest42" (if xen_4_4 || xen_4_5 || disable_xenguest then "disable" else "enable");
      Printf.sprintf "ENABLE_XENTOOLLOG=--%s-xentoollog" (if xentoollog then "enable" else "disable");
      Printf.sprintf "ENABLE_XENGUEST=%s" (if xenguest = None then "false" else "true");
      Printf.sprintf "XENGUEST_VERSION=%s" (match xenguest with Some x -> x | None -> "none");
      Printf.sprintf "HAVE_XEN_4_5=%s" (if xen_4_5 then "true" else "false");
      Printf.sprintf "HAVE_XEN_4_6=%s" (if xen_4_6 then "true" else "false");

    ] in
  output_file config_mk lines;
  (* Write config.h *)
  let lines =
    [ "/* Warning - this file is autogenerated by the configure script */";
      "/* Do not edit */";
      (if have_viridian then "" else "/* ") ^ "#define HAVE_HVM_PARAM_VIRIDIAN" ^ (if have_viridian then "" else " */");
      (if xen_4_5 then "" else "/* ") ^ "#define HAVE_XEN_4_5" ^ (if xen_4_5 then "" else " */");
      (if xen_4_6 then "" else "/* ") ^ "#define HAVE_XEN_4_6" ^ (if xen_4_6 then "" else " */");
      (if xc_domain_save_generation_id then "" else "/* ") ^ "#define HAVE_XC_DOMAIN_SAVE_GENERATION_ID" ^ (if xc_domain_save_generation_id then "" else " */");
      (if cores_per_socket then "" else "/* ") ^ "#define HAVE_CORES_PER_SOCKET" ^ (if cores_per_socket then "" else " */");
      (if domain_create_has_config then "" else "/*") ^ "#define DOMAIN_CREATE_HAS_CONFIG" ^ (if domain_create_has_config then "" else " */");
    ] in
  output_file config_h lines;
  (try Unix.unlink ("lib/" ^ config_h) with _ -> ());
  Unix.symlink ("../" ^ config_h) ("lib/" ^ config_h)

let arg =
  let doc = "enable verbose printing" in
  Arg.(value & flag & info ["verbose"; "v"] ~doc)

let configure_t = Term.(pure configure $ arg $ disable_xenguest)

let () =
  match
    Term.eval (configure_t, info)
  with
  | `Error _ -> exit 1
  | _ -> exit 0
