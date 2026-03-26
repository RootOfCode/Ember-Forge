(in-package #:ember-forge)

(defstruct upgrade-def
  (id :none :type keyword)
  (name "" :type string)
  (description "" :type string)
  (cost '() :type list) ; plist (:resource amount ...)
  (effect nil)
  (unlock nil))

(defparameter *upgrades*
  (list
   (make-upgrade-def
    :id :sturdy-gloves
    :name "Sturdy Gloves"
    :description "Double click mining power."
    :cost '(:coins 25.0d0 :stone 20.0d0)
    :effect (lambda (s) (setf (game-state-click-mult s) (* (game-state-click-mult s) 2.0d0)))
    :unlock (lambda (s) (>= (resource-ever s :stone) 20.0d0)))

   (make-upgrade-def
    :id :miners-pouch
    :name "Miner's Pouch"
    :description "Double manual gathering yield."
    :cost '(:coins 25.0d0 :stone 20.0d0)
    :effect (lambda (s) (setf (game-state-gather-mult s) (* (game-state-gather-mult s) 2.0d0)))
    :unlock (lambda (s) (>= (resource-ever s :stone) 20.0d0)))

   (make-upgrade-def
    :id :quick-hands
    :name "Quick Hands"
    :description "Manual mining and gathering are 0.5s faster."
    :cost '(:coins 60.0d0 :stone 50.0d0)
    :effect (lambda (s)
              (setf (game-state-manual-mine-duration s)
                    (max 0.5 (- (game-state-manual-mine-duration s) 0.5)))
              (setf (game-state-manual-gather-duration s)
                    (max 0.5 (- (game-state-manual-gather-duration s) 0.5))))
    :unlock (lambda (s) (>= (building-count s :pickaxe) 2)))

   (make-upgrade-def
    :id :better-picks
    :name "Hardened Pickaxes"
    :description "Pickaxes mine 2× faster."
    :cost '(:coins 100.0d0 :iron-b 5.0d0)
    :effect (lambda (s) (multiply-building-prod! s :pickaxe 2.0d0))
    :unlock (lambda (s) (>= (building-count s :pickaxe) 5)))

   (make-upgrade-def
    :id :coal-veins
    :name "Deep Coal Seams"
    :description "Coal pits produce 50% more coal."
    :cost '(:coins 300.0d0 :stone 50.0d0)
    :effect (lambda (s) (multiply-building-prod! s :coal-pit 1.5d0))
    :unlock (lambda (s) (>= (building-count s :coal-pit) 3)))

   (make-upgrade-def
    :id :steel-alloy
    :name "Steel Alloy Research"
    :description "Unlocks steel smelting recipe."
    :cost '(:coins 2000.0d0 :iron-b 20.0d0 :bronze 10.0d0)
    :effect (lambda (s) (declare (ignore s)))
    :unlock (lambda (s) (>= (resource-ever s :iron-b) 20.0d0)))

   (make-upgrade-def
    :id :smelt-slot-1
    :name "Second Crucible"
    :description "Adds one extra smelting slot."
    :cost '(:coins 800.0d0 :bronze 5.0d0)
    :effect (lambda (s) (incf (game-state-smelt-slots-extra s)))
    :unlock (lambda (s) (>= (building-count s :furnace) 1)))

   (make-upgrade-def
    :id :smelt-slot-2
    :name "Third Crucible"
    :description "Adds one extra smelting slot."
    :cost '(:coins 2500.0d0 :steel 5.0d0)
    :effect (lambda (s) (incf (game-state-smelt-slots-extra s)))
    :unlock (lambda (s) (upgrade-owned-p s :smelt-slot-1)))

   (make-upgrade-def
    :id :smelt-tuning
    :name "Furnace Tuning"
    :description "Smelting is 25% faster."
    :cost '(:coins 500.0d0 :iron-b 10.0d0)
    :effect (lambda (s) (setf (game-state-smelt-speed-mult s) (* (game-state-smelt-speed-mult s) 1.25d0)))
    :unlock (lambda (s) (>= (resource-ever s :iron-b) 5.0d0)))

   (make-upgrade-def
    :id :market-network
    :name "Market Network"
    :description "Market stalls generate 2× more coins."
    :cost '(:coins 600.0d0 :copper-b 10.0d0)
    :effect (lambda (s) (multiply-building-prod! s :market-stall 2.0d0))
    :unlock (lambda (s) (>= (building-count s :market-stall) 1)))

   (make-upgrade-def
    :id :deep-drilling
    :name "Deep-Core Drilling"
    :description "Unlocks Mythril Drill building."
    :cost '(:coins 20000.0d0 :steel 20.0d0 :gear 10.0d0)
    :effect (lambda (s) (declare (ignore s)))
    :unlock (lambda (s) (>= (resource-ever s :steel) 10.0d0)))

   (make-upgrade-def
    :id :ancient-veins
    :name "Ancient Veins"
    :description "+50% ore production permanently."
    :cost '(:coins 8000.0d0 :steel 10.0d0 :bronze 20.0d0)
    :effect (lambda (s)
              (multiply-building-prod! s :iron-mine 1.5d0)
              (multiply-building-prod! s :copper-mine 1.5d0)
              (multiply-building-prod! s :tinsmith 1.5d0)
              (multiply-building-prod! s :mythril-drill 1.5d0))
    :unlock (lambda (s) (>= (resource-ever s :steel) 5.0d0)))

   (make-upgrade-def
    :id :reinforced-sacks
    :name "Reinforced Sacks"
    :description "Stone/Coal storage +50%."
    :cost '(:coins 120.0d0 :stone 200.0d0 :iron-b 5.0d0)
    :effect (lambda (s)
              (setf (gethash :stone (game-state-caps s)) (* (resource-cap s :stone) 1.5d0))
              (setf (gethash :coal (game-state-caps s)) (* (resource-cap s :coal) 1.5d0)))
    :unlock (lambda (s) (>= (resource-ever s :stone) 200.0d0)))

   (make-upgrade-def
    :id :ore-crates
    :name "Ore Crates"
    :description "Ore storage +50%."
    :cost '(:coins 260.0d0 :gear 5.0d0 :bronze 5.0d0)
    :effect (lambda (s)
              (dolist (kw '(:iron :copper :tin))
                (setf (gethash kw (game-state-caps s)) (* (resource-cap s kw) 1.5d0))))
    :unlock (lambda (s) (>= (building-count s :iron-mine) 1)))

   (make-upgrade-def
    :id :market-signage
    :name "Bright Signage"
    :description "Market stalls generate +50% coins."
    :cost '(:coins 350.0d0 :copper-b 10.0d0 :stone 150.0d0)
    :effect (lambda (s) (multiply-building-prod! s :market-stall 1.5d0))
    :unlock (lambda (s) (>= (building-count s :market-stall) 1)))

   (make-upgrade-def
    :id :precision-molds
    :name "Precision Molds"
    :description "Gears, bellows, and rivets craft 2× output."
    :cost '(:coins 900.0d0 :bronze 20.0d0 :iron-b 20.0d0)
    :effect (lambda (s) (declare (ignore s)))
    :unlock (lambda (s) (>= (resource-ever s :gear) 10.0d0)))

   (make-upgrade-def
    :id :metalworking
    :name "Metalworking"
    :description "Unlocks Steel Plates and Rivets recipes."
    :cost '(:coins 1800.0d0 :steel 5.0d0 :bronze 10.0d0)
    :effect (lambda (s) (declare (ignore s)))
    :unlock (lambda (s) (>= (resource-ever s :steel) 5.0d0)))

   (make-upgrade-def
    :id :coin-minting
    :name "Coin Minting"
    :description "Unlocks the Trade Token minting recipe."
    :cost '(:coins 2500.0d0 :bronze 25.0d0 :gear 15.0d0)
    :effect (lambda (s) (declare (ignore s)))
    :unlock (lambda (s) (>= (building-count s :market-stall) 2)))

   (make-upgrade-def
    :id :industrial-tools
    :name "Industrial Tools"
    :description "Unlocks Machine Parts and boosts Mythril Drills +50%."
    :cost '(:coins 15000.0d0 :steel-plate 10.0d0 :gear 20.0d0 :rivet 50.0d0)
    :effect (lambda (s) (multiply-building-prod! s :mythril-drill 1.5d0))
    :unlock (lambda (s) (>= (resource-ever s :steel-plate) 5.0d0)))

   (make-upgrade-def
    :id :gear-driven-picks
    :name "Gear-Driven Picks"
    :description "Pickaxes mine 75% faster."
    :cost '(:coins 1200.0d0 :gear 50.0d0)
    :effect (lambda (s) (multiply-building-prod! s :pickaxe 1.75d0))
    :unlock (lambda (s) (>= (building-count s :pickaxe) 20)))

   (make-upgrade-def
    :id :coal-dust-filters
    :name "Coal Dust Filters"
    :description "Coal pits produce +75% coal."
    :cost '(:coins 900.0d0 :bronze 12.0d0 :gear 6.0d0)
    :effect (lambda (s) (multiply-building-prod! s :coal-pit 1.75d0))
    :unlock (lambda (s) (>= (building-count s :coal-pit) 10)))

   (make-upgrade-def
    :id :ore-sorting
    :name "Ore Sorting"
    :description "Iron/Copper mines and Tinsmith produce +40%."
    :cost '(:coins 3000.0d0 :bronze 20.0d0 :gear 10.0d0)
    :effect (lambda (s)
              (multiply-building-prod! s :iron-mine 1.4d0)
              (multiply-building-prod! s :copper-mine 1.4d0)
              (multiply-building-prod! s :tinsmith 1.4d0))
    :unlock (lambda (s) (>= (resource-ever s :bronze) 10.0d0)))

   (make-upgrade-def
    :id :bellows-efficiency
    :name "Bellows Efficiency"
    :description "Bellows Workshops grant 15% smelt speed each."
    :cost '(:coins 5000.0d0 :bellows 10.0d0 :steel 10.0d0)
    :effect (lambda (s) (declare (ignore s)))
    :unlock (lambda (s) (>= (building-count s :bellows-shop) 1)))

   (make-upgrade-def
    :id :assembly-lines
    :name "Assembly Lines"
    :description "All building production +15%."
    :cost '(:coins 8000.0d0 :machine-part 3.0d0 :rivet 150.0d0 :steel-plate 15.0d0)
    :effect (lambda (s) (setf (game-state-prod-mult s) (* (game-state-prod-mult s) 1.15d0)))
    :unlock (lambda (s) (>= (resource-ever s :machine-part) 1.0d0)))

   (make-upgrade-def
    :id :mechanized-drills
    :name "Mechanized Drills"
    :description "Mythril Drills produce 2× mythril."
   :cost '(:coins 75000.0d0 :machine-part 8.0d0 :steel 80.0d0)
   :effect (lambda (s) (multiply-building-prod! s :mythril-drill 2.0d0))
    :unlock (lambda (s) (>= (building-count s :mythril-drill) 1)))))

(defun find-upgrade-def (id)
  (find id *upgrades* :key #'upgrade-def-id :test #'eq))

(defun upgrade-owned-p (state id)
  (member id (game-state-upgrades state)))

(defun upgrade-available-p (state def)
  (let ((u (upgrade-def-unlock def)))
    (and (not (upgrade-owned-p state (upgrade-def-id def)))
         (if u (funcall u state) t))))

(defun buy-upgrade! (state id)
  (let ((def (find-upgrade-def id)))
    (when (and def (upgrade-available-p state def))
      (let ((cost (upgrade-def-cost def)))
        (when (can-afford-plist-p state cost)
          (spend-plist! state cost)
          (push id (game-state-upgrades state))
          (when (upgrade-def-effect def)
            (funcall (upgrade-def-effect def) state))
          (recompute-production! state)
          (add-notification! state (format nil "Upgrade: ~A" (upgrade-def-name def)) :color :gold)
          t)))))
