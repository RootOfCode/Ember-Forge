(in-package #:ember-forge)

(defvar *last-ticks* 0)

(defun reset-timer ()
  (setf *last-ticks* (sdl2:get-ticks)))

(defun compute-dt ()
  (let* ((now (sdl2:get-ticks))
         (dt (/ (- now *last-ticks*) 1000.0)))
    (setf *last-ticks* now)
    (coerce (min dt 0.5) 'single-float)))

