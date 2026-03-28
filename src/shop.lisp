(in-package #:ember-forge)

(defvar *shop-items* nil)

(defun shop-buy-mult (state)
  "Multiplier applied to a resource's base sell-value when buying from the shop.

Computed to avoid buy→sell arbitrage even with discounts/sell bonuses."
  (let* ((sell (game-state-sell-mult state))
         (disc (max 0.01d0 (game-state-shop-discount state)))
         (min-mult (/ (* sell 1.05d0) disc)))
    (max 2.5d0 min-mult)))

(defun shop-buy-cost (state resource qty)
  (let* ((q (max 0.0d0 (coerce qty 'double-float)))
         (base (sell-value resource))
         (disc (game-state-shop-discount state)))
    (* base q (shop-buy-mult state) disc)))

(defun shop-can-buy-resource-p (state resource qty)
  (and (not (eq resource :coins))
       (> (sell-value resource) 0.0d0)
       (let* ((q (max 0.0d0 (coerce qty 'double-float)))
              (amt (resource-amount state resource))
              (cap (resource-cap state resource))
              (room (- cap amt)))
         (and (> q 0.0d0)
              (>= room q)
              (resource-has-p state :coins (shop-buy-cost state resource q))))))

(defun buy-resource! (state resource qty)
  (when (and (not (eq resource :coins))
             (> (sell-value resource) 0.0d0))
    (let* ((q (max 0.0d0 (coerce qty 'double-float)))
           (amt (resource-amount state resource))
           (cap (resource-cap state resource))
           (room (max 0.0d0 (- cap amt)))
           (buy-q (min q room))
           (cost (shop-buy-cost state resource buy-q)))
      (when (and (> buy-q 0.0d0)
                 (resource-has-p state :coins cost))
        (resource-sub state :coins cost)
        (resource-add state resource buy-q)
        (add-notification! state (format nil "Bought ~A ~A"
                                         (fmt-number buy-q)
                                         (resource-name resource))
                           :color :gold)
        t))))

(defun shop-item-level (state id)
  (gethash id (game-state-shop-purchases state) 0))

(defun shop-item-owned-p (state id)
  (> (shop-item-level state id) 0))

(defstruct shop-item-def
  (id :none :type keyword)
  (name "" :type string)
  (description "" :type string)
  (base-cost 100.0d0 :type double-float) ; coins
  (cost-scale 1.6d0 :type double-float)
  (max-level 1 :type fixnum) ; 0 = unlimited
  (unlock nil)               ; NIL or (lambda (state) ...) -> boolean
  (apply nil))               ; NIL or (lambda (state new-level) ...)

(defun find-shop-item-def (id)
  (find id *shop-items* :key #'shop-item-def-id :test #'eq))

(defun shop-item-unlocked-p (state def)
  (let ((u (shop-item-def-unlock def)))
    (if u (funcall u state) t)))

(defun shop-item-maxed-p (state def)
  (let* ((id (shop-item-def-id def))
         (lvl (shop-item-level state id))
         (max (shop-item-def-max-level def)))
    (and (> max 0) (>= lvl max))))

(defun shop-item-next-cost (state def)
  (let* ((lvl (shop-item-level state (shop-item-def-id def)))
         (base (shop-item-def-base-cost def))
         (scale (shop-item-def-cost-scale def))
         (raw (* base (expt scale lvl)))
         (disc (game-state-shop-discount state))
         (cost (* raw disc)))
    (coerce (ceiling cost) 'double-float)))

(defun shop-item-available-p (state def)
  (and (shop-item-unlocked-p state def)
       (not (shop-item-maxed-p state def))))

(defun multiply-all-caps! (state factor)
  (let ((f (coerce factor 'double-float)))
    (dolist (kw (all-resource-keys) state)
      (setf (gethash kw (game-state-caps state))
            (* (resource-cap state kw) f))))
  state)

(defun buy-shop-item! (state id)
  (let ((def (find-shop-item-def id)))
    (when (and def (shop-item-available-p state def))
      (let ((cost (shop-item-next-cost state def)))
        (when (resource-has-p state :coins cost)
          (resource-sub state :coins cost)
          (let ((new-level (1+ (shop-item-level state id))))
            (setf (gethash id (game-state-shop-purchases state)) new-level)
            (when (shop-item-def-apply def)
              (funcall (shop-item-def-apply def) state new-level))
            (recompute-production! state)
            (add-notification! state (format nil "Shop: ~A" (shop-item-def-name def)) :color :gold)
            t))))))

(defshop-items
  (:haggling "Haggling Lessons"
   "Shop prices -5% (stacks)."
   :base-cost 120.0d0
   :scale 1.8d0
   :max 10
   :unlock (>= (resource-ever s :coins) 120.0d0)
   :apply (setf (game-state-shop-discount s)
                (max 0.60d0 (* (game-state-shop-discount s) 0.95d0))))

  (:salesmanship "Salesmanship"
   "Sell values +10% (stacks)."
   :base-cost 150.0d0
   :scale 1.75d0
   :max 12
   :unlock (>= (resource-ever s :coins) 150.0d0)
   :apply (setf (game-state-sell-mult s)
                (min 3.0d0 (* (game-state-sell-mult s) 1.10d0))))

  (:warehouse "Warehouse Lease"
   "All storage caps +15% (stacks)."
   :base-cost 220.0d0
   :scale 1.9d0
   :max 8
   :unlock (>= (resource-ever s :stone) 200.0d0)
   :apply (multiply-all-caps! s 1.15d0))

  (:smelters-coupon "Smelter's Coupon"
   "Smelting +10% speed (stacks)."
   :base-cost 400.0d0
   :scale 1.85d0
   :max 10
   :unlock (>= (resource-ever s :iron-b) 5.0d0)
   :apply (setf (game-state-smelt-speed-mult s)
                (* (game-state-smelt-speed-mult s) 1.10d0)))

  (:foreman-contract "Foreman Contract"
   "All building production +7% (stacks)."
   :base-cost 650.0d0
   :scale 2.0d0
   :max 10
   :unlock (>= (resource-ever s :bronze) 5.0d0)
   :apply (setf (game-state-prod-mult s)
                (* (game-state-prod-mult s) 1.07d0))))
