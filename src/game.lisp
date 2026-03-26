(in-package #:ember-forge)

(defun reset-transient-state! (state)
  (setf (game-state-manual-mine state) nil)
  (setf (game-state-manual-gather state) nil)
  (setf (game-state-active-event state) nil)
  (setf (game-state-notifications state) '())
  state)

(defun hard-reset-game! (state)
  "Full reset (wipes save-like progress)."
  (setf (game-state-tick state) 0)
  (setf (game-state-last-save-ut state) 0)
  (setf (game-state-dt state) 0.0)

  (setf (game-state-resources state) (make-hash-table :test 'eq))
  (setf (game-state-caps state) (make-hash-table :test 'eq))
  (setf (game-state-ever-produced state) (make-hash-table :test 'eq))
  (setf (game-state-unlocked state) (list :stone :coal :coins))
  (setf (game-state-production state) (make-hash-table :test 'eq))

  (setf (game-state-buildings state) (make-hash-table :test 'eq))
  (setf (game-state-building-mults state) (make-hash-table :test 'eq))

  (setf (game-state-discovered state) '())
  (setf (game-state-smelt-queue state) '())
  (setf (game-state-upgrades state) '())

  (setf (game-state-essence state) 0.0d0)
  (setf (game-state-ascensions state) 0)
  (setf (game-state-prod-mult state) 1.0d0)
  (setf (game-state-click-mult state) 1.0d0)
  (setf (game-state-gather-mult state) 1.0d0)
  (setf (game-state-smelt-speed-mult state) 1.0d0)

  (setf (game-state-eternal-upgrades state) '())
  (setf (game-state-achievements state) '())

  (setf (game-state-sell-mult state) 1.0d0)
  (setf (game-state-shop-discount state) 1.0d0)
  (setf (game-state-shop-purchases state) (make-hash-table :test 'eq))

  (setf (game-state-manual-mine-duration state) 3.0)
  (setf (game-state-manual-gather-duration state) 3.0)

  (setf (game-state-event-timer state) 90.0)
  (setf (game-state-active-event state) nil)
  (setf (game-state-smelt-boost-timer state) 0.0)
  (setf (game-state-iron-penalty-timer state) 0.0)

  (setf (game-state-active-panel state) :forge)
  (setf (game-state-notifications state) '())
  (setf (game-state-smelt-slots-extra state) 0)

  (setf (game-state-fps state) 0)
  (setf (game-state-frame-counter state) 0)
  (setf (game-state-frame-accum state) 0.0)

  ;; Starting resources
  (setf (gethash :coins (game-state-resources state)) 25.0d0)
  (setf (gethash :coins (game-state-ever-produced state)) 25.0d0)
  state)

(defun apply-offline-production! (state)
  (let* ((last (game-state-last-save-ut state))
         (now (get-universal-time)))
    (when (> last 0)
      (let* ((elapsed (min 28800 (max 0 (- now last)))) ; max 8 hours
             (sec (coerce elapsed 'double-float)))
        (when (> sec 1.0d0)
          (recompute-production! state)
          (maphash (lambda (res rate)
                     (resource-add state res (* rate sec)))
                   (game-state-production state))
          (add-notification! state (format nil "Offline: ~A seconds" elapsed) :color :text-dim))))))

(defun start-continue-game! (state)
  "Load save (if present) and enter gameplay."
  (init-resource-caps! state)
  (recompute-production! state)
  (load-save state)
  (reset-transient-state! state)
  (apply-eternal-start-bonuses! state)
  (recompute-production! state)
  (maybe-unlock-produced-resources! state)
  (when (game-state-offline-progress-enabled state)
    (apply-offline-production! state))
  (setf (game-state-screen state) :game)
  (setf (game-state-game-started-p state) t)
  state)

(defun start-new-game! (state)
  "Hard-reset and enter gameplay."
  (hard-reset-game! state)
  (init-resource-caps! state)
  (recompute-production! state)
  (setf (game-state-screen state) :game)
  (setf (game-state-game-started-p state) t)
  state)
