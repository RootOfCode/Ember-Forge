(in-package #:ember-forge)

(eval-when (:compile-toplevel :load-toplevel :execute)
  (defun %dsl-even-plist-p (plist)
    (evenp (length plist)))

  (defun %dsl-plist-get1 (plist &rest keys)
    "Return the first present value among KEYS in PLIST, else NIL."
    (dolist (k keys nil)
      (when (getf plist k)
        (return (getf plist k)))))

  (defun %dsl-plist-member-p (plist key)
    "Like (GETF PLIST KEY), but can distinguish missing vs NIL value."
    (loop for (k v) on plist by #'cddr
          when (eq k key) do (return (values t v))
          finally (return (values nil nil))))

  (defun %dsl-plist-remove-keys (plist keys)
    (loop for (k v) on plist by #'cddr
          unless (member k keys :test #'eq)
            append (list k v)))

  (defun %dsl-wrap-lambda (lambda-list form)
    (cond
      ((null form) nil)
      ((and (consp form) (member (car form) '(lambda function) :test #'eq)) form)
      (t `(lambda ,lambda-list ,form))))

  (defun %dsl-wrap-lambda-ignore (lambda-list ignored form)
    (cond
      ((null form) nil)
      ((and (consp form) (member (car form) '(lambda function) :test #'eq)) form)
      (t `(lambda ,lambda-list
            (declare (ignore ,@ignored))
            ,form))))

  (defun %dsl-quote-plist (form)
    (cond
      ((null form) nil)
      ((and (consp form) (eq (car form) 'quote)) form)
      (t `(quote ,form)))))

(defmacro defresources (&body entries)
  "Define resources and sell values.

Syntax:
  (defresources
    (:stone \"Stone\" :color :gray :tier 0 :cap 500.0d0 :sell 0.5d0)
    (:coins \"Coins\" :color :gold :tier 0 :cap 1.0d6))"
  (let ((defs '())
        (sells '()))
    (dolist (entry entries)
      (destructuring-bind (id name &rest plist) entry
        (unless (keywordp id)
          (error "defresources: id must be a keyword, got ~S" id))
        (unless (stringp name)
          (error "defresources: name must be a string for ~S, got ~S" id name))
        (unless (%dsl-even-plist-p plist)
          (error "defresources: odd plist for ~S: ~S" id plist))

        (multiple-value-bind (sell-present sell-val) (%dsl-plist-member-p plist :sell)
          (when (and sell-present (not (null sell-val)))
            (setf sells (nconc sells (list id sell-val)))))

        (let* ((props (%dsl-plist-remove-keys plist '(:sell :name)))
               (cap (or (getf props :base-cap)
                        (getf props :cap)))
               (props (if cap
                          (nconc (%dsl-plist-remove-keys props '(:cap :base-cap))
                                 (list :base-cap cap))
                          (%dsl-plist-remove-keys props '(:cap :base-cap)))))
          (setf defs (nconc defs (list id `(list :name ,name ,@props)))))))
    `(progn
       (defparameter *resource-defs*
         (a:plist-hash-table (list ,@defs)))
       (defparameter *sell-values*
         (a:plist-hash-table (list ,@sells))))))

(defmacro defbuildings (&body entries)
  "Define *buildings*.

Syntax:
  (defbuildings
    (:pickaxe \"Rusty Pickaxe\" \"...\" :cost (:coins 10.0d0) :produces (:stone 0.5d0))
    ...)"
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name description &rest plist) entry
               (unless (keywordp id)
                 (error "defbuildings: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defbuildings: name must be a string for ~S, got ~S" id name))
               (unless (stringp description)
                 (error "defbuildings: description must be a string for ~S, got ~S" id description))
               (unless (%dsl-even-plist-p plist)
                 (error "defbuildings: odd plist for ~S: ~S" id plist))

               (let* ((cost (%dsl-plist-get1 plist :base-cost :cost))
                      (scale (%dsl-plist-get1 plist :cost-scale :scale))
                      (prod (%dsl-plist-get1 plist :production :produces))
                      (unlock (getf plist :unlock)))
                 `(make-building-def
                   :id ,id
                   :name ,name
                   :description ,description
                   ,@(when cost (list :base-cost (%dsl-quote-plist cost)))
                   ,@(when scale (list :cost-scale scale))
                   ,@(when prod (list :production (%dsl-quote-plist prod)))
                   ,@(when unlock (list :unlock (%dsl-wrap-lambda '(s) unlock)))))))
           entries)))
    `(defparameter *buildings*
       (list ,@expanded))))

(defmacro defrecipes (&body entries)
  "Define *recipes*.

Syntax:
  (defrecipes
    (:iron-bar \"Smelt Iron Bar\" :in (:iron 3.0d0 :coal 1.0d0) :out (:iron-b 1.0d0) :time 4.0)
    ...)"
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name &rest plist) entry
               (unless (keywordp id)
                 (error "defrecipes: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defrecipes: name must be a string for ~S, got ~S" id name))
               (unless (%dsl-even-plist-p plist)
                 (error "defrecipes: odd plist for ~S: ~S" id plist))
               (let* ((inputs (%dsl-plist-get1 plist :inputs :in))
                      (outputs (%dsl-plist-get1 plist :outputs :out))
                      (duration (%dsl-plist-get1 plist :duration :time))
                      (unlock (getf plist :unlock)))
                 `(make-recipe-def
                   :id ,id
                   :name ,name
                   ,@(when inputs (list :inputs (%dsl-quote-plist inputs)))
                   ,@(when outputs (list :outputs (%dsl-quote-plist outputs)))
                   ,@(when duration (list :duration duration))
                   ,@(when unlock (list :unlock-condition (%dsl-wrap-lambda '(s) unlock)))))))
           entries)))
    `(defparameter *recipes*
       (list ,@expanded))))

(defmacro defupgrades (&body entries)
  "Define *upgrades*.

Syntax:
  (defupgrades
    (:sturdy-gloves \"Sturdy Gloves\" \"...\" :cost (:coins 25.0d0) :unlock (>= ...) :effect (setf ...))
    ...)"
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name description &rest plist) entry
               (unless (keywordp id)
                 (error "defupgrades: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defupgrades: name must be a string for ~S, got ~S" id name))
               (unless (stringp description)
                 (error "defupgrades: description must be a string for ~S, got ~S" id description))
               (unless (%dsl-even-plist-p plist)
                 (error "defupgrades: odd plist for ~S: ~S" id plist))
               (let* ((cost (getf plist :cost))
                      (effect (getf plist :effect))
                      (unlock (getf plist :unlock)))
                 `(make-upgrade-def
                   :id ,id
                   :name ,name
                   :description ,description
                   ,@(when cost (list :cost (%dsl-quote-plist cost)))
                   ,@(when effect (list :effect (%dsl-wrap-lambda '(s) effect)))
                   ,@(when unlock (list :unlock (%dsl-wrap-lambda '(s) unlock)))))))
           entries)))
    `(defparameter *upgrades*
       (list ,@expanded))))

(defmacro defshop-items (&body entries)
  "Define *shop-items*."
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name description &rest plist) entry
               (unless (keywordp id)
                 (error "defshop-items: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defshop-items: name must be a string for ~S, got ~S" id name))
               (unless (stringp description)
                 (error "defshop-items: description must be a string for ~S, got ~S" id description))
               (unless (%dsl-even-plist-p plist)
                 (error "defshop-items: odd plist for ~S: ~S" id plist))
               (let* ((base-cost (getf plist :base-cost))
                      (scale (%dsl-plist-get1 plist :cost-scale :scale))
                      (max (%dsl-plist-get1 plist :max-level :max))
                      (unlock (getf plist :unlock))
                      (apply (getf plist :apply)))
                 `(make-shop-item-def
                   :id ,id
                   :name ,name
                   :description ,description
                   ,@(when base-cost (list :base-cost base-cost))
                   ,@(when scale (list :cost-scale scale))
                   ,@(when max (list :max-level max))
                   ,@(when unlock (list :unlock (%dsl-wrap-lambda '(s) unlock)))
                   ,@(when apply (list :apply (%dsl-wrap-lambda-ignore '(s lvl) '(lvl) apply)))))))
           entries)))
    `(defparameter *shop-items*
       (list ,@expanded))))

(defmacro defeternal-upgrades (&body entries)
  "Define *eternal-upgrades*."
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name description &rest plist) entry
               (unless (keywordp id)
                 (error "defeternal-upgrades: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defeternal-upgrades: name must be a string for ~S, got ~S" id name))
               (unless (stringp description)
                 (error "defeternal-upgrades: description must be a string for ~S, got ~S" id description))
               (unless (%dsl-even-plist-p plist)
                 (error "defeternal-upgrades: odd plist for ~S: ~S" id plist))
               (let ((cost (getf plist :cost))
                     (effect (getf plist :effect)))
                 `(make-eternal-upgrade-def
                   :id ,id
                   :name ,name
                   :description ,description
                   ,@(when cost (list :cost cost))
                   ,@(when effect (list :effect (%dsl-wrap-lambda '(s) effect)))))))
           entries)))
    `(defparameter *eternal-upgrades*
       (list ,@expanded))))

(defmacro choice (label &rest rest)
  "Create an EVENT-CHOICE.

Syntax:
  (choice \"Label\" :can <expr> <body...>)

The state variable is named S inside <expr>/<body...>."
  (unless (stringp label)
    (error "choice: label must be a string, got ~S" label))
  (let ((can nil)
        (body rest))
    (when (and (consp body) (eq (car body) :can))
      (unless (consp (cdr body))
        (error "choice: missing value for :can in ~S" (cons label rest)))
      (setf can (cadr body)
            body (cddr body)))
    (let ((effect (when body `(lambda (s) ,@body)))
          (can-fn (%dsl-wrap-lambda '(s) can)))
      `(%event-choice ,label ,effect ,@(when can-fn (list :can can-fn))))))

(defmacro defevents (&body entries)
  "Define *events*.

Syntax:
  (defevents
    (:cave-in \"Cave-In!\" \"Text...\" (choice ...) (choice ...))
    ...)"
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name text &body choices) entry
               (unless (keywordp id)
                 (error "defevents: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defevents: name must be a string for ~S, got ~S" id name))
               (unless (stringp text)
                 (error "defevents: text must be a string for ~S, got ~S" id text))
               `(make-event-def
                 :id ,id
                 :name ,name
                 :text ,text
                 :choices (list ,@choices))))
           entries)))
    `(defparameter *events*
       (list ,@expanded))))

(defmacro defachievements (&body entries)
  "Define *achievements*.

Syntax:
  (defachievements
    (:first-spark \"First Spark\" \"Smelt your first bar.\" (or ...))
    ...)"
  (let ((expanded
          (mapcar
           (lambda (entry)
             (destructuring-bind (id name description unlock) entry
               (unless (keywordp id)
                 (error "defachievements: id must be a keyword, got ~S" id))
               (unless (stringp name)
                 (error "defachievements: name must be a string for ~S, got ~S" id name))
               (unless (stringp description)
                 (error "defachievements: description must be a string for ~S, got ~S" id description))
               `(make-achievement-def
                 :id ,id
                 :name ,name
                 :description ,description
                 :unlock ,(%dsl-wrap-lambda '(s) unlock))))
           entries)))
    `(defparameter *achievements*
       (list ,@expanded))))
