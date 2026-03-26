(in-package #:ember-forge)

(defparameter *panel-background-files*
  '((:menu . "assets/backgrounds/main_menu.png")
    (:forge . "assets/backgrounds/forge.png")
    (:mine . "assets/backgrounds/mine.png")
    (:upgrades . "assets/backgrounds/upgrades.png")
    (:recipes . "assets/backgrounds/recipes.png")
    (:shop . "assets/backgrounds/shop.png")
    (:prestige . "assets/backgrounds/prestige.png")))

(defvar *panel-background-textures* (make-hash-table :test 'eq))

(defun clear-panel-backgrounds! ()
  (maphash (lambda (k tex)
             (declare (ignore k))
             (ignore-errors (sdl2:destroy-texture tex)))
           *panel-background-textures*)
  (clrhash *panel-background-textures*)
  t)

(defun %find-asset (relative)
  (or (probe-file relative)
      (probe-file (merge-pathnames relative (uiop:getcwd)))
      (ignore-errors (probe-file (asdf:system-relative-pathname :ember-forge relative)))))

(defun panel-background-file (panel)
  (cdr (assoc panel *panel-background-files* :test #'eq)))

(defun ensure-panel-background-texture (ren panel)
  (or (gethash panel *panel-background-textures*)
      (let* ((rel (panel-background-file panel))
             (path (and rel (%find-asset rel))))
        (when path
          (handler-case
              (let ((surface (sdl2-image:load-image (namestring path))))
                (unwind-protect
                    (let ((tex (sdl2:create-texture-from-surface ren surface)))
                      (ignore-errors (sdl2:set-texture-blend-mode tex :blend))
                      (setf (gethash panel *panel-background-textures*) tex)
                      tex)
                  (ignore-errors (autowrap:autocollect-cancel surface))
                  (sdl2:free-surface surface)))
            (error ()
              nil))))))

(defun draw-panel-background (ren panel x y w h &key (overlay '(0 0 0 160)))
  (let ((tex (ensure-panel-background-texture ren panel)))
    (cond
      (tex
       (sdl2:render-copy ren tex
                         :source-rect (cffi:null-pointer)
                         :dest-rect (sdl2:make-rect x y w h))
       (draw-rect ren x y w h overlay :border-color :btn-hover))
      (t
       (draw-rect ren x y w h :bg-panel :border-color :btn-hover)))))
