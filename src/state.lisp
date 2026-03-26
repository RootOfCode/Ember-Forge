(in-package #:ember-forge)

(defconstant +window-width+ 1280)
(defconstant +window-height+ 768)

(defstruct game-state
  (tick 0 :type fixnum)
  (last-save-ut 0 :type (integer 0 *))
  (dt 0.0 :type single-float)

  (resources (make-hash-table :test 'eq) :type hash-table)
  (caps (make-hash-table :test 'eq) :type hash-table)
  (ever-produced (make-hash-table :test 'eq) :type hash-table)
  (unlocked (list :stone :coal :coins) :type list)
  (production (make-hash-table :test 'eq) :type hash-table)

  (buildings (make-hash-table :test 'eq) :type hash-table)
  (building-mults (make-hash-table :test 'eq) :type hash-table)

  (discovered '() :type list)
  (smelt-queue '() :type list)
  (upgrades '() :type list)

  (essence 0.0d0 :type double-float)
  (ascensions 0 :type fixnum)
  (prod-mult 1.0d0 :type double-float)
  (click-mult 1.0d0 :type double-float)
  (gather-mult 1.0d0 :type double-float)
  (smelt-speed-mult 1.0d0 :type double-float)

  ;; Persistent (prestige) upgrades purchased with essence.
  (eternal-upgrades '() :type list)

  ;; Economy modifiers
  (sell-mult 1.0d0 :type double-float)
  (shop-discount 1.0d0 :type double-float)
  (shop-purchases (make-hash-table :test 'eq) :type hash-table)

  (manual-mine-duration 3.0 :type single-float)
  (manual-gather-duration 3.0 :type single-float)
  (manual-mine nil)
  (manual-gather nil)

  ;; Random events + timed modifiers
  (event-timer 90.0 :type single-float)
  (active-event nil) ; keyword or NIL
  (smelt-boost-timer 0.0 :type single-float)
  (iron-penalty-timer 0.0 :type single-float)

  ;; Achievements (purely cosmetic)
  (achievements '() :type list)

  ;; Options/config (persisted separately from save slots)
  (save-slot 1 :type fixnum)
  (autosave-enabled t)
  (autosave-interval 60 :type fixnum)
  (offline-progress-enabled t)
  (fullscreen-enabled nil)
  (font-path "assets/fonts/JetBrainsMono/fonts/ttf/JetBrainsMono-Regular.ttf" :type string)

  ;; Global UI mode (title/pause menu vs gameplay)
  (screen :menu :type keyword) ; :menu :options :game
  (game-started-p nil) ; session-only

  (active-panel :forge :type keyword)
  (notifications '() :type list)

  (smelt-slots-extra 0 :type fixnum)

  ;; FPS tracking (for UI only)
  (fps 0 :type fixnum)
  (frame-counter 0 :type fixnum)
  (frame-accum 0.0 :type single-float))

(defstruct building-instance
  (id :none :type keyword)
  (count 0 :type fixnum)
  (level 1 :type fixnum))

(defstruct smelt-job
  (recipe :none :type keyword)
  (progress 0.0 :type single-float)
  (duration 5.0 :type single-float)
  (source :manual :type keyword)) ; :manual or :auto

(defstruct notif
  (text "" :type string)
  (timer 3.0 :type single-float)
  (color :gold :type keyword))

(defstruct manual-job
  (resource :stone :type keyword)
  (amount 1.0d0 :type double-float)
  (timer 0.0 :type single-float)
  (duration 3.0 :type single-float))

(defun manual-job-progress (job)
  (if (and job (> (manual-job-duration job) 0.0))
      (clamp (- 1.0 (/ (manual-job-timer job) (manual-job-duration job))) 0.0 1.0)
      0.0))

(defun start-manual-mine! (state)
  (when (null (game-state-manual-mine state))
    (let ((dur (max 0.1 (game-state-manual-mine-duration state))))
      (setf (game-state-manual-mine state)
            (make-manual-job :resource :stone
                             :amount (coerce (game-state-click-mult state) 'double-float)
                             :timer (coerce dur 'single-float)
                             :duration (coerce dur 'single-float))))
    t))

(defun start-manual-gather! (state resource)
  (when (null (game-state-manual-gather state))
    (let ((dur (max 0.1 (game-state-manual-gather-duration state))))
      (setf (game-state-manual-gather state)
            (make-manual-job :resource resource
                             :amount (coerce (game-state-gather-mult state) 'double-float)
                             :timer (coerce dur 'single-float)
                             :duration (coerce dur 'single-float))))
    t))

(defun tick-manual-jobs! (state dt)
  (let ((job (game-state-manual-mine state)))
    (when job
      (decf (manual-job-timer job) (coerce dt 'single-float))
      (when (<= (manual-job-timer job) 0.0)
        (resource-add state (manual-job-resource job) (manual-job-amount job))
        (add-notification! state (format nil "+~A ~A"
                                         (fmt-number (manual-job-amount job))
                                         (resource-name (manual-job-resource job)))
                           :color :green-ok)
        (setf (game-state-manual-mine state) nil))))
  (let ((job (game-state-manual-gather state)))
    (when job
      (decf (manual-job-timer job) (coerce dt 'single-float))
      (when (<= (manual-job-timer job) 0.0)
        (resource-add state (manual-job-resource job) (manual-job-amount job))
        (add-notification! state (format nil "+~A ~A"
                                         (fmt-number (manual-job-amount job))
                                         (resource-name (manual-job-resource job)))
                           :color :green-ok)
        (setf (game-state-manual-gather state) nil))))
  state)

(defun make-initial-state ()
  (let ((s (make-game-state)))
    (setf (gethash :coins (game-state-resources s)) 25.0d0)
    (setf (gethash :stone (game-state-resources s)) 0.0d0)
    (setf (gethash :coal (game-state-resources s)) 0.0d0)
    (setf (gethash :coins (game-state-ever-produced s)) 25.0d0)
    s))
