(in-package #:ember-forge)

(defstruct building-def
  (id :none :type keyword)
  (name "" :type string)
  (description "" :type string)
  (base-cost '() :type list)   ; plist (:resource amount ...)
  (cost-scale 1.15d0 :type double-float)
  (production '() :type list)  ; plist (:resource rate-per-sec ...)
  (unlock nil))

(defbuildings
  (:pickaxe "Rusty Pickaxe"
   "Mines stone automatically. Slow, but it never sleeps."
   :cost (:coins 10.0d0)
   :produces (:stone 0.5d0))

  (:coal-pit "Coal Pit"
   "Digs coal from the earth. Requires stone infrastructure."
   :cost (:coins 50.0d0 :stone 20.0d0)
   :produces (:coal 0.3d0))

  (:iron-mine "Iron Mine"
   "Extracts iron ore from deep veins."
   :cost (:coins 200.0d0 :stone 100.0d0)
   :produces (:iron 0.2d0)
   :unlock (>= (resource-ever s :stone) 50.0d0))

  (:copper-mine "Copper Mine"
   "Extracts copper ore."
   :cost (:coins 250.0d0 :stone 120.0d0)
   :produces (:copper 0.2d0)
   :unlock (>= (resource-ever s :stone) 50.0d0))

  (:tinsmith "Tinsmith"
   "Refines tin for alloying."
   :cost (:coins 400.0d0 :copper-b 5.0d0)
   :produces (:tin 0.15d0))

  (:furnace "Auto-Furnace"
   "Automatically runs iron bar smelt jobs; adds one smelting slot."
   :cost (:coins 500.0d0 :stone 200.0d0 :gear 5.0d0)
   :unlock (>= (building-count s :iron-mine) 1))

  (:bellows-shop "Bellows Workshop"
   "Speeds up all smelting by 10% per workshop."
   :cost (:coins 1000.0d0 :bronze 10.0d0 :bellows 3.0d0))

  (:market-stall "Market Stall"
   "Generates coins from trade and scrap."
   :cost (:coins 80.0d0 :stone 60.0d0 :coal 20.0d0)
   :produces (:coins 0.35d0)
   :unlock (>= (resource-ever s :stone) 100.0d0))

  (:mythril-drill "Mythril Drill"
   "Bores deep into ancient rock for mythril."
   :cost (:coins 50000.0d0 :steel 50.0d0 :gear 30.0d0)
   :produces (:mythril 0.05d0)
   :unlock (upgrade-owned-p s :deep-drilling)))

(defun find-building-def (id)
  (find id *buildings* :key #'building-def-id :test #'eq))

(defun ensure-building-instance (state id)
  (or (gethash id (game-state-buildings state))
      (setf (gethash id (game-state-buildings state))
            (make-building-instance :id id :count 0 :level 1))))

(defun building-count (state id)
  (building-instance-count (ensure-building-instance state id)))

(defun building-prod-mult (state id)
  (let* ((base (gethash id (game-state-building-mults state) 1.0d0))
         (veins (if (and (member :eternal-ancient-veins (game-state-eternal-upgrades state))
                          (member id '(:iron-mine :copper-mine :tinsmith :mythril-drill)))
                    1.5d0
                    1.0d0)))
    (* base veins)))

(defun multiply-building-prod! (state id factor)
  (setf (gethash id (game-state-building-mults state))
        (* (building-prod-mult state id) (coerce factor 'double-float)))
  state)

(defun building-unlocked-p (state def)
  (let ((u (building-def-unlock def)))
    (or (> (building-count state (building-def-id def)) 0)
        (if u (funcall u state) t))))

(defun building-cost (def owned)
  (let ((scale (building-def-cost-scale def))
        (out '()))
    (loop for (res amt) on (building-def-base-cost def) by #'cddr
          do (let* ((base (coerce amt 'double-float))
                    (cost (* base (expt scale owned))))
               (setf out (nconc out (list res cost)))))
    out))

(defun can-afford-plist-p (state plist)
  (loop for (res amt) on plist by #'cddr
        always (resource-has-p state res amt)))

(defun spend-plist! (state plist)
  (loop for (res amt) on plist by #'cddr
        do (resource-sub state res amt))
  state)

(defun recompute-production! (state)
  (let ((prod (make-hash-table :test 'eq)))
    (dolist (def *buildings*)
      (let* ((id (building-def-id def))
             (n (building-count state id))
             (mult (* (building-prod-mult state id) (game-state-prod-mult state))))
        (when (> n 0)
          (loop for (res rate) on (building-def-production def) by #'cddr
                do (incf (gethash res prod 0.0d0)
                         (* n (coerce rate 'double-float) mult))))))
    (setf (game-state-production state) prod))
  state)

(defun tick-buildings (state dt)
  (let* ((ddt (coerce dt 'double-float))
         (iron-mult (if (> (game-state-iron-penalty-timer state) 0.0)
                        0.9d0
                        1.0d0)))
    (maphash (lambda (res rate)
               (resource-add state res
                             (* (coerce rate 'double-float)
                                ddt
                                (if (eq res :iron) iron-mult 1.0d0))))
             (game-state-production state)))
  state)

(defun max-smelt-slots (state)
  (+ 5
     (game-state-smelt-slots-extra state)
     (building-count state :furnace)))

(defun smelt-speed-mult (state)
  (let ((pulse (if (> (game-state-smelt-boost-timer state) 0.0)
                   2.0d0
                   1.0d0))
        (mastery (if (member :forge-mastery (game-state-eternal-upgrades state))
                     1.25d0
                     1.0d0)))
    (* (game-state-smelt-speed-mult state)
       pulse
       mastery
       (+ 1.0d0 (* (if (upgrade-owned-p state :bellows-efficiency) 0.15d0 0.10d0)
                   (building-count state :bellows-shop))))))

(defun buy-building! (state id)
  (let ((def (find-building-def id)))
    (when (and def (building-unlocked-p state def))
      (let* ((inst (ensure-building-instance state id))
             (owned (building-instance-count inst))
             (cost (building-cost def owned)))
        (when (can-afford-plist-p state cost)
          (spend-plist! state cost)
          (incf (building-instance-count inst))
          (recompute-production! state)
          (add-notification! state (format nil "Built: ~A" (building-def-name def)) :color :green-ok)
          t)))))
