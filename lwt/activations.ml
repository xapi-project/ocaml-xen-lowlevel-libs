
let nr_events = 1024
let event_cb = Array.init nr_events (fun _ -> Lwt_sequence.create ())

let wait port =
  let th, u = Lwt.task () in
  let node = Lwt_sequence.add_r u event_cb.(Eventchn.to_int port) in
  Lwt.on_cancel th (fun _ -> Lwt_sequence.remove node);
  th

let wake port =
	Lwt_sequence.iter_node_l (fun node ->
		let u = Lwt_sequence.get node in
		Lwt_sequence.remove node;
		Lwt.wakeup u ()
    ) event_cb.(port)

(* Go through the event mask and activate any events, potentially spawning
   new threads *)
let run () =
	let xe = Eventchn.init () in
	let fd = Lwt_unix.of_unix_file_descr ~blocking:false ~set_flags:true (Eventchn.fd xe) in
	let rec inner () =
		lwt () = Lwt_unix.wait_read fd in
	    let port = Eventchn.pending xe in
		wake (Eventchn.to_int port);
		Eventchn.unmask xe port;
        inner ()
   in inner ()
