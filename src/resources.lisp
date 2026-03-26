(in-package #:ember-forge)

(defparameter *resource-defs*
  (a:plist-hash-table
   (list
    :stone    (list :name "Stone"         :color :gray   :tier 0 :base-cap 500.0d0)
    :coal     (list :name "Coal"          :color :dark   :tier 0 :base-cap 300.0d0)
    :coins    (list :name "Coins"         :color :gold   :tier 0 :base-cap 1.0d6)

    :iron     (list :name "Iron Ore"      :color :rust   :tier 1 :base-cap 200.0d0)
    :copper   (list :name "Copper Ore"    :color :orange :tier 1 :base-cap 200.0d0)
    :tin      (list :name "Tin Ore"       :color :silver :tier 1 :base-cap 200.0d0)

    :iron-b   (list :name "Iron Bar"      :color :silver :tier 2 :base-cap 100.0d0)
    :copper-b (list :name "Copper Bar"    :color :orange :tier 2 :base-cap 100.0d0)

    :bronze   (list :name "Bronze Bar"    :color :bronze :tier 3 :base-cap 80.0d0)
    :gear     (list :name "Gear"          :color :gray   :tier 3 :base-cap 200.0d0)
    :bellows  (list :name "Bellows"       :color :brown  :tier 3 :base-cap 50.0d0)

    :steel    (list :name "Steel Bar"     :color :blue   :tier 4 :base-cap 60.0d0)
    :steel-plate (list :name "Steel Plate"   :color :blue   :tier 4 :base-cap 80.0d0)
    :rivet    (list :name "Rivets"        :color :silver :tier 3 :base-cap 400.0d0)
    :machine-part (list :name "Machine Part" :color :gray   :tier 5 :base-cap 50.0d0)
    :mythril  (list :name "Mythril Ore"   :color :cyan   :tier 5 :base-cap 40.0d0)
    :ember    (list :name "Ember Crystal" :color :red    :tier 6 :base-cap 10.0d0))))

(defun resource-def (kw)
  (gethash kw *resource-defs*))

(defun resource-name (kw)
  (getf (resource-def kw) :name (string kw)))

(defun resource-color (kw)
  (getf (resource-def kw) :color :text))

(defun resource-tier (kw)
  (getf (resource-def kw) :tier 0))

(defun resource-base-cap (kw)
  (getf (resource-def kw) :base-cap 0.0d0))

(defparameter *sell-values*
  (a:plist-hash-table
   (list
    :stone 0.5d0
    :coal 0.8d0
    :iron 1.2d0
    :copper 1.2d0
    :tin 1.5d0
    :iron-b 5.0d0
    :copper-b 5.0d0
    :bronze 10.0d0
    :steel 20.0d0
    :steel-plate 35.0d0
    :gear 8.0d0
    :bellows 25.0d0
    :rivet 2.0d0
    :machine-part 120.0d0
    :mythril 50.0d0
    :ember 200.0d0)))

(defun sell-value (kw)
  (gethash kw *sell-values* 0.0d0))

(defun can-sell-p (state kw qty)
  (and (not (eq kw :coins))
       (> (sell-value kw) 0.0d0)
       (resource-has-p state kw qty)))

(defun sell-resource! (state kw qty)
  (when (can-sell-p state kw qty)
    (let* ((q (coerce qty 'double-float))
           (coins (* (sell-value kw) q (game-state-sell-mult state))))
      (resource-sub state kw q)
      (resource-add state :coins coins)
      (add-notification! state (format nil "Sold ~A ~A"
                                       (fmt-number q)
                                       (resource-name kw))
                         :color :gold)
      t)))

(defun init-resource-caps! (state)
  (maphash (lambda (kw def)
             (declare (ignore def))
             (setf (gethash kw (game-state-caps state)) (resource-base-cap kw)))
           *resource-defs*)
  state)

(defun resource-cap (state kw)
  (or (gethash kw (game-state-caps state))
      (let ((cap (resource-base-cap kw)))
        (setf (gethash kw (game-state-caps state)) cap)
        cap)))

(defun resource-amount (state kw)
  (gethash kw (game-state-resources state) 0.0d0))

(defun resource-ever (state kw)
  (gethash kw (game-state-ever-produced state) 0.0d0))

(defun (setf resource-amount) (val state kw)
  (setf (gethash kw (game-state-resources state)) (coerce val 'double-float)))

(defun resource-unlock! (state kw)
  (unless (member kw (game-state-unlocked state))
    (push kw (game-state-unlocked state)))
  state)

(defun resource-unlocked-p (state kw)
  (member kw (game-state-unlocked state)))

(defun resource-has-p (state kw amount)
  (>= (resource-amount state kw) (coerce amount 'double-float)))

(defun resource-add (state kw delta)
  (let* ((d (coerce delta 'double-float))
         (before (resource-amount state kw))
         (cap (resource-cap state kw))
         (after (clamp (+ before d) 0.0d0 cap)))
    (setf (resource-amount state kw) after)
    (when (> after 0.0d0)
      (resource-unlock! state kw))
    (let ((inc (- after before)))
      (when (> inc 0.0d0)
        (setf (gethash kw (game-state-ever-produced state))
              (+ (resource-ever state kw) inc))))
    after))

(defun resource-sub (state kw delta)
  (resource-add state kw (- delta)))

(defun maybe-unlock-produced-resources! (state)
  (maphash (lambda (kw amt)
             (when (> (coerce amt 'double-float) 0.0d0)
               (resource-unlock! state kw)))
           (game-state-resources state))
  state)

(defun all-resource-keys ()
  (let ((out '()))
    (maphash (lambda (k v) (declare (ignore v)) (push k out)) *resource-defs*)
    (sort out #'< :key #'resource-tier)))
