(in-package #:ember-forge)

(defvar *ui-mouse-x* 0)
(defvar *ui-mouse-y* 0)
(defvar *ui-click* nil)
(defvar *ui-click-used* nil)
(defvar *ui-input-blocked* nil)

(defvar *ui-tooltip-text* nil)
(defvar *ui-tooltip-x* nil)
(defvar *ui-tooltip-y* nil)
(defvar *ui-tooltip-placement* :mouse) ; :mouse or :above

(defun ui-begin-frame (&key mouse-x mouse-y clicked)
  (setf *ui-mouse-x* mouse-x
        *ui-mouse-y* mouse-y
        *ui-click* clicked
        *ui-click-used* nil
        *ui-tooltip-text* nil
        *ui-tooltip-x* nil
        *ui-tooltip-y* nil
        *ui-tooltip-placement* :mouse))

(defun point-in-rect-p (x y rx ry rw rh)
  (and (>= x rx) (>= y ry) (< x (+ rx rw)) (< y (+ ry rh))))

(defun rect-hovered-p (x y w h)
  (point-in-rect-p *ui-mouse-x* *ui-mouse-y* x y w h))

(defun ui-take-click ()
  (when (and *ui-click* (not *ui-click-used*))
    (setf *ui-click-used* t)
    t))

(defun ui-set-tooltip (text &key x y (placement :mouse))
  (when (and text (plusp (length text)))
    (setf *ui-tooltip-text* text
          *ui-tooltip-x* x
          *ui-tooltip-y* y
          *ui-tooltip-placement* placement)))

(defun set-render-color* (ren color)
  (cond
    ((keywordp color)
     (multiple-value-bind (r g b a) (palette-rgba color)
       (sdl2:set-render-draw-color ren r g b a)))
    ((and (consp color) (= (length color) 4))
     (destructuring-bind (r g b a) color
       (sdl2:set-render-draw-color ren r g b a)))
    (t
     (sdl2:set-render-draw-color ren 255 0 255 255))))

(defun draw-rect (ren x y w h fill-color &key (border-color nil))
  (set-render-color* ren fill-color)
  (sdl2:render-fill-rect ren (sdl2:make-rect x y w h))
  (when border-color
    (set-render-color* ren border-color)
    (sdl2:render-draw-rect ren (sdl2:make-rect x y w h))))

(defun draw-label (ren font text x y color &key (align :left))
  (multiple-value-bind (r g b a) (palette-rgba color)
    (let* ((surface (sdl2-ttf:render-utf8-blended font (or text "") r g b a))
           (w (sdl2:surface-width surface))
           (h (sdl2:surface-height surface))
           (tx (ecase align
                 (:left x)
                 (:center (- x (floor w 2)))
                 (:right (- x w))))
           (ty y)
           (texture (sdl2:create-texture-from-surface ren surface)))
      (unwind-protect
          (sdl2:render-copy ren texture
                            :source-rect (cffi:null-pointer)
                            :dest-rect (sdl2:make-rect tx ty w h))
        (sdl2:destroy-texture texture)
        (autowrap:autocollect-cancel surface)
        (sdl2:free-surface surface)))))

(defun ui-split-lines (text)
  (let* ((s (or text ""))
         (len (length s))
         (out '())
         (start 0))
    (loop
      for pos = (position #\Newline s :start start)
      do (progn
           (push (subseq s start (or pos len)) out)
           (if pos
               (setf start (1+ pos))
               (return (nreverse out)))))))

(defun draw-multiline-label (ren font text x y color &key (line-height 18) (align :left))
  (let ((lines (ui-split-lines text)))
    (loop for line in lines
          for i from 0
          do (draw-label ren font line x (+ y (* i line-height)) color :align align))))

(defun text-width (font text)
  (multiple-value-bind (w h) (sdl2-ttf:size-utf8 font (or text ""))
    (declare (ignore h))
    w))

(defun fit-text-to-width (font text max-width &key (ellipsis "…"))
  (let* ((s (or text ""))
         (mw (max 0 (truncate max-width))))
    (cond
      ((or (<= mw 0) (zerop (length s))) "")
      ((<= (text-width font s) mw) s)
      (t
       (let* ((ell (or ellipsis ""))
              (ell-w (text-width font ell)))
         (when (>= ell-w mw)
           (return-from fit-text-to-width ""))
         (let ((low 0)
               (high (length s))
               (best 0))
           (loop while (<= low high)
                 do (let* ((mid (floor (+ low high) 2))
                           (candidate (concatenate 'string (subseq s 0 mid) ell)))
                      (if (<= (text-width font candidate) mw)
                          (progn
                            (setf best mid)
                            (setf low (1+ mid)))
                          (setf high (1- mid)))))
           (concatenate 'string (subseq s 0 best) ell)))))))

(defun draw-label-fit (ren font text x y color max-width &key (align :left) (ellipsis "…"))
  (draw-label ren font (fit-text-to-width font text max-width :ellipsis ellipsis) x y color :align align))

(defun draw-progress-bar (ren x y w h progress fill-color bg-color)
  (draw-rect ren x y w h bg-color :border-color :btn-hover)
  (let ((pw (round (* w (clamp progress 0.0 1.0)))))
    (when (> pw 0)
      (draw-rect ren x y pw h fill-color))))

(defun draw-button (ren font text x y w h &key (enabled t) (color :btn-normal) tooltip)
  (let* ((hovered (rect-hovered-p x y w h))
         (fill (cond
                 ((not enabled) :bg-panel)
                 ((and (not *ui-input-blocked*) hovered *ui-click*) :btn-active)
                 (hovered :btn-hover)
                 (t color)))
         (clicked (and (not *ui-input-blocked*)
                       enabled
                       hovered
                       (ui-take-click))))
    (when (and (not *ui-input-blocked*) hovered tooltip)
      (ui-set-tooltip tooltip))
    (draw-rect ren x y w h fill :border-color (if hovered :btn-active :btn-hover))
    (draw-label ren font text (+ x 10) (+ y 6) (if enabled :text :text-dim))
    clicked))

(defun draw-resource-icon (ren resource-kw x y)
  (let ((c (resource-color resource-kw)))
    (draw-rect ren x y 12 12 c :border-color :text-dim)))
