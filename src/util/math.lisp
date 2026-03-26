(in-package #:ember-forge)

(defun clamp (x min max)
  (max min (min max x)))

(defun lerp (a b tval)
  (+ a (* (- b a) tval)))

(defparameter *number-suffixes*
  ;; Thousand, Million, Billion, Trillion, Quadrillion, Quintillion, Sextillion...
  #("K" "M" "B" "T" "Qa" "Qi" "Sx" "Sp" "Oc" "No" "Dc"))

(defun fmt-number (n)
  "Format a double-float for UI display."
  (let* ((dn (coerce n 'double-float))
         (absn (abs dn)))
    (cond
      ((< absn 1000.0d0) (format nil "~,1F" dn))
      ((< absn 1.0d36)
       (let* ((exp3 (floor (/ (log absn 10) 3)))
              (idx (max 1 exp3))
              (suffix-idx (1- idx))
              (scaled (/ dn (expt 1000.0d0 idx))))
         (format nil "~,2F~A"
                 scaled
                 (aref *number-suffixes* (min suffix-idx (1- (length *number-suffixes*)))))))
      (t
       (format nil "~,3E" dn)))))

(defun fmt-rate (n)
  (format nil "~A/s" (fmt-number n)))

