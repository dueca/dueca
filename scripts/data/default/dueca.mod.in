;; -*-scheme-*-
;; this is an example dueca.mod file, for you to start out with and adapt
;; according to your needs. Note that you need a dueca.mod file only for the
;; node with number 0

;; NOTE: for new guile (from openSUSE 12.1 onwards), you may only once use
;; (define ...). For modifying an already defined value, used (set! ...)

;; in general, it is a good idea to clearly document your set up
;; this is an excellent place.
;; node set-up
(define ecs-node 0)    ; dutmms1, send order 3
;(define aux-node 1)   ; dutmms3, send order 1
;(define pfd-node 2)   ; dutmms5, send order 2
;(define cl-node 3)    ; dutmms4, send order 0

;; priority set-up
;; normal nodes: 0 administration
;;               1 hdf5 logging
;;               2 simulation, unpackers
;;               3 communication
;;               4 ticker

; administration priority. Run the interface and logging here
(define admin-priority (make-priority-spec 0 0))

;; log priority
(define log-priority (make-priority-spec 1 0))

; priority of simulation, just above log
(define sim-priority (make-priority-spec 2 0))

; nodes with a different priority scheme
; control loading node has 0, 1 and 2 as above and furthermore
;               4 stick priority
;               5 ticker priority
; priority of the stick. Higher than prio of communication
;(define stick-priority (make-priority-spec 4 0))

; timing set-up
; timing of the stick calculations. Assuming 100 usec ticks, this gives 2000 Hz
(define stick-timing (make-time-spec 0 5))

; this is normally 100, giving 100 Hz timing
(define sim-timing (make-time-spec 0 100))

;; for now, display on 50 Hz
(define display-timing (make-time-spec 0 200))

;; log a bit more economical, 25 Hz
(define log-timing (make-time-spec 0 400))

;;; the modules needed for dueca itself
(dueca-list
  (make-entity "dueca"
               (if (equal? 0 this-node-id)
                   (list
                    (make-module 'dusime "" admin-priority)
                    (make-module 'dueca-view "" admin-priority)
                    (make-module 'activity-view "" admin-priority)
                    (make-module 'timing-view "" admin-priority)
                    (make-module 'log-view "" admin-priority)
                    (make-module 'channel-view "" admin-priority)

                    ;; uncomment for web-based graph
                    ;; (make-module 'config-storage "" admin_priority))

                    ;; uncomment and adapt for initial condition
                    ;; (make-module
                    ;; 'initials-inventory "ph-simple" admin-priority
                    ;; 'reference-file "initials-ph-simple.toml"
                    ;; 'store-file "initials-ph-simple-%Y%m%d_%H%M.toml")

                    ;; (make-module
                    ;; 'replay-master "ph-simple" admin-priority
                    ;; 'reference-file "recordings-ph-simple.ddff"
                    ;; 'store-file "recordings-ph-simple-%Y%m%d_%H%M.ddff")
                    )
                 (list)
                 )
               (if (and (equal? 0 this-node-id)
                        (> 1 no-of-nodes)
                        (not classic-ip))
                   (list
                    (make-module 'net-view "" admin-priority)
                    )
                   (list)
                   )
               )
  )

;;; the modules for your application(s)
(define citation
  (make-entity "ph-simple"
               (if (equal? ecs-node this-node-id)
                   (list
                    (make-module 'some-module "" sim-priority
                                 'set-timing sim-timing
                                 'check-timing 10000 20000)

                    ;; example components, uncomment and adapt if needed
                    ;; web socket server
                    ;; (make-module 'web-sockets-server "" admin-priority
                    ;;              'set-timing sim-timing
                    ;;              'check-timing 10000 20000
                    ;;              'port 8001
                    ;;              'info "endpoint" "MyData://ph-simple"
                    ;;              'write-and-read "plotconfig"
                    ;;              "ConfigFileRequest://dueca"
                    ;;              "ConfigFileData://dueca"
                    ;;              'http-port 8000
                    ;;              'document-root
                    ;;              "/usr/share/dplotter/dist")
                    ;; hdf5 logging
                    ;; (make-module 'hdf5-logger "" log-priority
                    ;;              'set-timing log-timing
                    ;;              'chunksize 3000
                    ;;              'log-entry "MyData://ph-simple"
                    ;;              "MyData" "/data/mydata")
                    )

                 )

               ; an empty list; at least one list should be supplied
               ; for nodes that have no modules as argument
               (list)
               )
  )

