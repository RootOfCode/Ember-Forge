(in-package #:ember-forge)

(defstruct eternal-upgrade-def
  (id :none :type keyword)
  (name "" :type string)
  (description "" :type string)
  (cost 0.0d0 :type double-float) ; essence cost
  (effect nil))

(defparameter *eternal-upgrades*
  (list
   (make-eternal-upgrade-def
    :id :eternal-miner
    :name "Eternal Miner"
    :description "Start each run with 1 free Pickaxe."
    :cost 5.0d0
    :effect (lambda (s)
              (apply-eternal-start-bonuses! s)))
   (make-eternal-upgrade-def
    :id :ember-memory
    :name "Ember Memory"
    :description "Keep 10% of coins on Ascend."
    :cost 10.0d0
    :effect (lambda (s) (declare (ignore s))))
   (make-eternal-upgrade-def
    :id :forge-mastery
    :name "Forge Mastery"
    :description "All smelting is 25% faster permanently."
    :cost 25.0d0
    :effect (lambda (s) (declare (ignore s))))
   (make-eternal-upgrade-def
    :id :eternal-ancient-veins
    :name "Ancient Veins"
    :description "+50% ore production permanently."
    :cost 50.0d0
    :effect (lambda (s) (recompute-production! s)))
   (make-eternal-upgrade-def
    :id :twin-furnace
    :name "Twin Furnaces"
    :description "Start each run with an Auto-Furnace built."
    :cost 100.0d0
    :effect (lambda (s)
              (apply-eternal-start-bonuses! s)))))

(defun find-eternal-upgrade-def (id)
  (find id *eternal-upgrades* :key #'eternal-upgrade-def-id :test #'eq))

(defun eternal-upgrade-owned-p (state id)
  (member id (game-state-eternal-upgrades state)))

(defun apply-eternal-start-bonuses! (state)
  (when (eternal-upgrade-owned-p state :eternal-miner)
    (let ((inst (ensure-building-instance state :pickaxe)))
      (when (< (building-instance-count inst) 1)
        (setf (building-instance-count inst) 1))))
  (when (eternal-upgrade-owned-p state :twin-furnace)
    (let ((inst (ensure-building-instance state :furnace)))
      (when (< (building-instance-count inst) 1)
        (setf (building-instance-count inst) 1))))
  state)

(defun buy-eternal-upgrade! (state id)
  (let ((def (find-eternal-upgrade-def id)))
    (when def
      (let ((owned (eternal-upgrade-owned-p state id))
            (cost (eternal-upgrade-def-cost def)))
        (when (and (not owned) (>= (game-state-essence state) cost))
          (decf (game-state-essence state) cost)
          (push id (game-state-eternal-upgrades state))
          (when (eternal-upgrade-def-effect def)
            (funcall (eternal-upgrade-def-effect def) state))
          (recompute-production! state)
          (add-notification! state (format nil "Eternal: ~A" (eternal-upgrade-def-name def)) :color :red)
          t)))))

(defun prestige-available-p (state)
  (>= (resource-ever state :ember) 1.0d0))

(defun essence-gain (state)
  (let* ((ember (max 0.0d0 (resource-ever state :ember)))
         (base (floor (sqrt ember)))
         (bonus (* (game-state-ascensions state) 2)))
    (+ base bonus)))

(defun recompute-prestige-mults! (state)
  (let ((e (game-state-essence state)))
    (setf (game-state-prod-mult state) (+ 1.0d0 (* e 0.05d0)))
    (setf (game-state-click-mult state) (+ 1.0d0 (* e 0.02d0)))
    (setf (game-state-gather-mult state) (+ 1.0d0 (* e 0.015d0)))
    (setf (game-state-smelt-speed-mult state) (+ 1.0d0 (* e 0.01d0))))
  state)

(defun ascend! (state)
  "Prestige reset (keeps essence/ascensions)."
  (when (prestige-available-p state)
    (let* ((gain (essence-gain state))
           (coins-before (resource-amount state :coins))
           (kept-coins (if (eternal-upgrade-owned-p state :ember-memory)
                           (* 0.10d0 coins-before)
                           0.0d0))
           (start-coins (+ 25.0d0 kept-coins)))
      (incf (game-state-essence state) (coerce gain 'double-float))
      (incf (game-state-ascensions state))
      (recompute-prestige-mults! state)
      (setf (game-state-manual-mine-duration state) 3.0)
      (setf (game-state-manual-gather-duration state) 3.0)

      (setf (game-state-resources state) (make-hash-table :test 'eq))
      (setf (game-state-caps state) (make-hash-table :test 'eq))
      (setf (game-state-ever-produced state) (make-hash-table :test 'eq))
      (setf (game-state-production state) (make-hash-table :test 'eq))
      (setf (game-state-buildings state) (make-hash-table :test 'eq))
      (setf (game-state-building-mults state) (make-hash-table :test 'eq))
      (setf (game-state-upgrades state) '())
      (setf (game-state-discovered state) '())
      (setf (game-state-smelt-queue state) '())
      (setf (game-state-unlocked state) (list :stone :coal :coins))
      (setf (game-state-smelt-slots-extra state) 0)
      (setf (game-state-manual-mine state) nil)
      (setf (game-state-manual-gather state) nil)
      (setf (game-state-active-panel state) :forge)

      (setf (game-state-active-event state) nil)
      (setf (game-state-event-timer state) 90.0)
      (setf (game-state-smelt-boost-timer state) 0.0)
      (setf (game-state-iron-penalty-timer state) 0.0)

      ;; Reset run-scoped shop modifiers.
      (setf (game-state-sell-mult state) 1.0d0)
      (setf (game-state-shop-discount state) 1.0d0)
      (setf (game-state-shop-purchases state) (make-hash-table :test 'eq))

      (init-resource-caps! state)
      (resource-add state :coins start-coins)
      (apply-eternal-start-bonuses! state)
      (recompute-production! state)
      (add-notification! state (format nil "Ascended (+~D Essence)" gain) :color :red)
      t)))
