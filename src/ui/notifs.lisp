(in-package #:ember-forge)

(defun add-notification! (state text &key (timer 3.0) (color :gold))
  (push (make-notif :text text :timer (coerce timer 'single-float) :color color)
        (game-state-notifications state))
  state)

(defun tick-notifications! (state dt)
  (let ((out '()))
    (dolist (n (game-state-notifications state))
      (decf (notif-timer n) (coerce dt 'single-float))
      (when (> (notif-timer n) 0.0)
        (push n out)))
    (setf (game-state-notifications state) (nreverse out)))
  state)

(defun render-notifications (ren font state x y w h)
  (draw-rect ren x y w h :bg-top :border-color :btn-hover)
  (let ((msg (car (game-state-notifications state))))
    (when msg
      (draw-label ren font (notif-text msg) (+ x 10) (+ y 6) (notif-color msg)))))
