;; document the set-up of the simulation

;; nodes
(define ecs-node 0) ; ecs,                      dutmms1, send order 1
(define  cl-node 1) ; control loading           dutmms4, send order 0
(define pfd-node 2) ; primary flight display    dutmms2, send order 2

;; default priority set-up
(define admin-priority (make-priority-spec 0 0))  ; admin, interface, logging
(define sim-priority   (make-priority-spec 1 0))  ; simulation, unpacking

;; difference; control loading
(if (equal? this-node-id cl-node)
    (list
     (define stick-priority (make-priority-spec 3 0))  ; cl simulation
     )
  )

;; timing parameters, basis is a granule size of 500 us
(define stick-timing (make-time-spec 0 1))      ; cl calculation, 2000 Hz
(define sim-timing (make-time-spec 0 40))       ; sim, 50 Hz
(define feedback-timing (make-time-spec 0 400)) ; eval feedback, 5 Hz

;;; the modules needed for dueca itself
;;; the modules needed for dueca itself
(dueca-list
  (make-entity "dueca"
               (if (equal? 0 this-node-id)
                   (list
                    (make-module 'dusime "" admin-priority 'min-interval 4000)
                    (make-module 'dueca-view "" admin-priority)
                    (make-module 'activity-view "" admin-priority)
                    (make-module 'timing-view "" admin-priority)
                    )
                 (list)
                 )))

;;; the spaceplane
(define spaceplane
  (make-entity "spaceplane"
               (if (equal? this-node-id ecs-node)
                   (list
                    (make-module 'space-plane "" sim-priority
                                 'set-timing sim-timing 'set-stop-height 0.6)
                    (make-module 'numeric-plane-output "" sim-priority
                                 'set-output-file "dump")
                    (make-module 'evaluator "" subsim-priority
                                 'set-path "path1.txt"
                                 'set-flare 7331 'set-final 3879
                                 'set-path "path2.txt"
                                 'set-flare 9280 'set-final 4673
                                 'set-path "path3.txt"
                                 'set-flare 11416 'set-final 5971
                                 'set-feedback-timing feedback-timing
                                 'set-stop-height 0.6)
                    (make-module 'spaceplane-control "" admin-priority)
                    )
                 )
               (if (equal? this-node-id 1)
                   (list
                    (make-module 'mmslab-stick "stick" stick-priority
                                 'set-timing stick-timing)
                    (make-module 'rate-reduction "" stick-priority
                                 'set-timing sim-timing)
                    )
                 )
               (if (equal? this-node-id 2)
                   (list
                    (make-module 'display-space "" admin-priority
                                 'set-fullscreen #t)
                    )
                 )
               (list)   ; for nodes that get no modules, empty list
               )) ; spaceplane-complete

;; define Lisp mode for this file, for emacs
;; Local Variables: **
;; mode: lisp **
;; fill-column: 75 **
;; comment-column: 0 **
;; End:
