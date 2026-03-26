(in-package #:ember-forge)

(defvar *available-fonts* nil)

(defun %find-path (relative)
  (or (probe-file relative)
      (probe-file (merge-pathnames relative (uiop:getcwd)))
      (ignore-errors (probe-file (asdf:system-relative-pathname :ember-forge relative)))))

(defun %find-dir (relative)
  (let ((p (%find-path relative)))
    (when p
      (uiop:ensure-directory-pathname p))))

(defun %font-file-p (path)
  (let ((ext (and (pathname-type path) (string-downcase (pathname-type path)))))
    (member ext '("ttf" "otf") :test #'string=)))

(defun %collect-font-files (dir)
  (let ((out '()))
    (dolist (f (ignore-errors (uiop:directory-files dir)))
      (when (%font-file-p f)
        (push f out)))
    (dolist (sub (ignore-errors (uiop:subdirectories dir)))
      (setf out (nconc out (%collect-font-files sub))))
    out))

(defun %rel-namestring (path)
  (handler-case
      (let* ((cwd (uiop:getcwd))
             (rel (enough-namestring path cwd)))
        (if (and rel (plusp (length rel)))
            rel
            (namestring path)))
    (error ()
      (namestring path))))

(defun scan-fonts! ()
  "Scan `assets/fonts/` and `fonts/` (recursively) for .ttf/.otf files.
Stores relative namestrings when possible."
  (let ((files '()))
    (dolist (root (remove nil (list (%find-dir "assets/fonts/")
                                    (%find-dir "fonts/"))))
      (setf files (nconc files (%collect-font-files root))))
    (setf *available-fonts*
          (sort (remove-duplicates (mapcar #'%rel-namestring files) :test #'string=)
                #'string<))
    *available-fonts*))

(defun available-fonts ()
  (or *available-fonts* (scan-fonts!)))

(defun resolve-font-file (choice)
  (when (and (stringp choice) (plusp (length choice)))
    (let ((p (%find-path choice)))
      (when p (truename p)))))

(defun font-display-name (choice)
  (let ((p (ignore-errors (pathname choice))))
    (if p
        (file-namestring p)
        (or choice ""))))

(defun default-font-choice ()
  (let* ((fonts (available-fonts))
         (preferred "assets/fonts/Roboto-Mono/Roboto-Mono-regular.ttf"))
    (or (and (member preferred fonts :test #'string=) preferred)
        (first fonts)
        preferred)))

