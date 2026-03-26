(in-package #:cl-user)

(require :asdf)

(defun %getenv (name)
  (let ((v (uiop:getenv name)))
    (and (stringp v) (plusp (length v)) v)))

(defun %truthy-env-p (name &key (default t))
  (let ((v (%getenv name)))
    (cond
      ((null v) default)
      (t (not (member (string-downcase v) '("0" "false" "no" "off" "nil") :test #'string=))))))

(let* ((quicklisp-setup
         (or (let ((env (%getenv "QUICKLISP_SETUP")))
               (and env (ignore-errors (uiop:ensure-pathname env :want-file t :want-absolute t))))
             (merge-pathnames "quicklisp/setup.lisp" (user-homedir-pathname))))
       (out (or (%getenv "EMBER_FORGE_OUT")
                (if (uiop:os-windows-p)
                    "dist/ember-forge.exe"
                    "dist/ember-forge")))
       (compression (%truthy-env-p "EMBER_FORGE_COMPRESSION" :default t)))
  (format t "~&[Ember Forge] Quicklisp: ~A~%" (namestring quicklisp-setup))
  (unless (probe-file quicklisp-setup)
    (format *error-output* "~&Quicklisp not found at: ~A~%" (namestring quicklisp-setup))
    (format *error-output* "~&Set QUICKLISP_SETUP to your setup.lisp path and retry.~%")
    (uiop:quit 1))

  (load quicklisp-setup)
  (require :asdf)

  ;; Quicklisp isn't available at READ-time for this file, so avoid `ql:...` reader syntax.
  ;; If present, use QL:QUICKLOAD to auto-install missing deps (e.g. SDL2-IMAGE) before ASDF loads the system.
  (let* ((ql-pkg (find-package "QL"))
         (quickload (and ql-pkg (find-symbol "QUICKLOAD" ql-pkg))))
    (when quickload
      (format t "~&[Ember Forge] Quicklisp quickload deps...~%")
      ;; On Windows/Wine, ensure CFFI searches our local `dist/` folder for DLLs.
      (handler-case
          (funcall quickload :cffi :silent t)
        (error (e)
          (format *error-output* "~&[Ember Forge] WARNING: quickload ~A failed: ~A~%" :cffi e)))
      (let* ((cffi-pkg (find-package "CFFI"))
             (dirs-sym (and cffi-pkg (find-symbol "*FOREIGN-LIBRARY-DIRECTORIES*" cffi-pkg)))
             (cwd (uiop:getcwd))
             (dist-dir (ignore-errors (uiop:ensure-directory-pathname (merge-pathnames "dist/" cwd)))))
        (when (and dirs-sym (boundp dirs-sym) dist-dir)
          (pushnew dist-dir (symbol-value dirs-sym) :test #'equal)))

      (dolist (sys '(:alexandria :sdl2 :sdl2-ttf :sdl2-image))
        (handler-case
            (funcall quickload sys :silent t)
          (error (e)
            (format *error-output* "~&[Ember Forge] WARNING: quickload ~A failed: ~A~%" sys e))))))

  (asdf:load-asd (truename "ember-forge.asd"))
  (asdf:load-system :ember-forge)

  (format t "~&[Ember Forge] Saving executable: ~A~%" out)
  (let* ((pkg (or (find-package :ember-forge)
                  (error "EMBER-FORGE package not found after loading system.")))
         (main-sym (or (find-symbol "MAIN" pkg)
                       (error "EMBER-FORGE:MAIN not found after loading system."))))
    (sb-ext:save-lisp-and-die out
                              :toplevel main-sym
                              :executable t
                              :compression compression)))
