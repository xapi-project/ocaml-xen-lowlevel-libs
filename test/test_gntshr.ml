open Gnt

let main () =
  let shr_h = Gntshr.interface_open () in
  let dev_h = Gnttab.interface_open () in
  let share = Gntshr.share_pages_exn shr_h 0 1 true in
  List.iter (fun r -> Printf.printf "Shared a page with gntref = %d\n%!" (int_of_grant_table_index r)) Gntshr.(share.refs);
  Printf.printf "Shared page(s) OK. Now trying to map.\n%!";
  let local_mapping = Gnttab.map_exn dev_h Gnttab.({domid=0; ref=List.hd Gntshr.(share.refs)}) true in
  Printf.printf "Mapping OK! Now unmapping and unsharing everything.\n%!";
  Gnttab.unmap_exn dev_h local_mapping;
  Gntshr.munmap_exn shr_h share;
  Printf.printf "Now trying to share and map 10 pages as a vector.\n%!";
  let share = Gntshr.share_pages_exn shr_h 0 10 true in
  let refs = Gntshr.(share.refs) in
  let grants = List.map (fun ref -> Gnttab.({domid=0; ref})) refs in
  let local_mapping = Gnttab.mapv_exn dev_h grants true in
  Printf.printf "Success! Now unmapping and unsharing everything!\n%!";
  Gnttab.unmap_exn dev_h local_mapping;
  Gntshr.munmap_exn shr_h share

let _ = main ()
