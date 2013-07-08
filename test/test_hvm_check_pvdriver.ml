let _ =
	let domid = int_of_string Sys.argv.(1) in
	let pvdriver = Xenctrl.with_intf (fun xc -> Xenctrl.hvm_check_pvdriver xc domid) in
	Printf.printf "Domain %d has pvdriver: %b\n" domid pvdriver
