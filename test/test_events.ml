let (>>=) = Lwt.bind

let main () =
  (* Obtain a handler. *)
  let evtchn_h = Eventchn.init () in

  (* Create two connected event channels. *)
  let ch1 = Eventchn.bind_unbound_port evtchn_h 0 in
  let ch2 = Eventchn.bind_interdomain evtchn_h 0 (Eventchn.to_int ch1) in

  Printf.printf "ch1 = %d, ch2 = %d\n%!" (Eventchn.to_int ch1) (Eventchn.to_int ch2); 

  let th = Activations.wait ch2 in
  th >>= fun () ->

  (* Wait on ch2. *)
  let th = Activations.wait ch2 in

  (* Notify ch1. *)
  Eventchn.notify evtchn_h ch1;

  (* th should wake up! *)
  th >>= fun () -> Printf.printf "Success!\n%!"; Lwt.return ()

let _ = Lwt_main.run (main ())
