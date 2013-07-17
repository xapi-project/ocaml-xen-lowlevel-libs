open Gnt
open Gnt.Gntshr

let main () =
  let h = interface_open () in
  let share = share_pages_exn h 0 1 true in
  List.iter (fun r -> Printf.printf "%d\n%!" (int_of_grant_table_index r)) share.refs;
  munmap_exn h share

let _ = main ()
