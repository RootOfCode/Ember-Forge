(in-package #:ember-forge)

(defparameter *options-version* 4)

(defun options-path ()
  (let ((env (uiop:getenv "EMBER_FORGE_OPTIONS_PATH")))
    (cond
      ((and env (plusp (length env)))
       (uiop:ensure-pathname env :want-absolute t :want-file t))
      (t
       (merge-pathnames
        (make-pathname :directory '(:relative "saves")
                       :name "options"
                       :type "lisp")
        (uiop:getcwd))))))

(defun fallback-options-path ()
  (merge-pathnames "ember-forge-options.lisp" (uiop:getcwd)))

(defun %normalize-save-slot (x)
  (let ((n (if (and (integerp x) (> x 0)) x 1)))
    (max 1 (min 3 n))))

(defun %normalize-autosave-interval (x)
  (let ((n (if (and (integerp x) (> x 0)) x 60)))
    (max 10 (min 3600 n))))

(defun options-to-plist (state)
  (list
   :version *options-version*
   :saved-at (get-universal-time)
   :save-slot (game-state-save-slot state)
   :autosave-enabled (not (null (game-state-autosave-enabled state)))
   :autosave-interval (game-state-autosave-interval state)
   :offline-progress-enabled (not (null (game-state-offline-progress-enabled state)))
   :fullscreen-enabled (not (null (game-state-fullscreen-enabled state)))
   :font-path (game-state-font-path state)))

(defun apply-options-plist! (state pl)
  (labels ((g (k &optional default) (getf pl k default)))
    (setf (game-state-save-slot state)
          (%normalize-save-slot (g :save-slot (game-state-save-slot state))))
    (setf (game-state-autosave-enabled state)
          (not (null (g :autosave-enabled (game-state-autosave-enabled state)))))
    (setf (game-state-autosave-interval state)
          (%normalize-autosave-interval
           (g :autosave-interval (game-state-autosave-interval state))))
    (setf (game-state-offline-progress-enabled state)
          (not (null (g :offline-progress-enabled (game-state-offline-progress-enabled state)))))
    (setf (game-state-fullscreen-enabled state)
          (not (null (g :fullscreen-enabled (game-state-fullscreen-enabled state)))))
    (let ((fp (g :font-path nil)))
      (when (and (stringp fp) (plusp (length fp)))
        (setf (game-state-font-path state) fp))))
  state)

(defun save-options (state)
  (labels ((try (path)
             (ensure-directories-exist path)
             (with-open-file (f path :direction :output :if-exists :supersede)
               (prin1 (options-to-plist state) f))
             path))
    (handler-case
        (try (options-path))
      (error ()
        (handler-case
            (try (fallback-options-path))
          (error () nil))))))

(defun load-options! (state)
  (labels ((try (path)
             (when (probe-file path)
               (with-open-file (f path)
                 (let ((pl (read f nil nil)))
                   (when (listp pl)
                     (apply-options-plist! state pl)
                     t))))))
    (ignore-errors (or (try (options-path))
                       (try (fallback-options-path))
                       nil)))
  state)
