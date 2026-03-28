(in-package #:ember-forge)

(defstruct event-def
  (id :none :type keyword)
  (name "" :type string)
  (text "" :type string)
  (choices '() :type list)) ; list of EVENT-CHOICE structs

(defstruct event-choice
  (label "" :type string)
  (can nil)   ; NIL or (lambda (state) ...) -> boolean
  (effect nil))

(defun find-event-def (id)
  (find id *events* :key #'event-def-id :test #'eq))

(defun %event-choice (label effect &key can)
  (make-event-choice :label label :can can :effect effect))

(defun schedule-next-event! (state)
  (setf (game-state-event-timer state)
        (coerce (+ 300 (random 600)) 'single-float))
  state)

(defun start-random-event! (state)
  (when (and (null (game-state-active-event state))
             (plusp (length *events*)))
    (let* ((idx (random (length *events*)))
           (ev (nth idx *events*)))
      (setf (game-state-active-event state) (event-def-id ev))
      (add-notification! state (format nil "Event: ~A" (event-def-name ev)) :color :warn)
      t)))

(defun resolve-active-event! (state)
  (when (game-state-active-event state)
    (setf (game-state-active-event state) nil)
    (schedule-next-event! state)
    t))

(defun choose-active-event! (state choice-index)
  (let* ((id (game-state-active-event state))
         (ev (and id (find-event-def id))))
    (when ev
      (let* ((choices (event-def-choices ev))
             (choice (nth choice-index choices)))
        (when choice
          (let ((can (event-choice-can choice)))
            (when (and can (not (funcall can state)))
              (add-notification! state "You can't do that right now." :color :warn)
              (return-from choose-active-event! nil)))
          (when (event-choice-effect choice)
            (funcall (event-choice-effect choice) state))
          (resolve-active-event! state)
          t)))))

(defun smelt-boost-active-p (state)
  (> (game-state-smelt-boost-timer state) 0.0))

(defun iron-penalty-active-p (state)
  (> (game-state-iron-penalty-timer state) 0.0))

(defun tick-events! (state dt)
  ;; Timed modifiers
  (when (smelt-boost-active-p state)
    (decf (game-state-smelt-boost-timer state) (coerce dt 'single-float))
    (when (<= (game-state-smelt-boost-timer state) 0.0)
      (setf (game-state-smelt-boost-timer state) 0.0)
      (add-notification! state "Ember Pulse faded." :color :text-dim)))

  (when (iron-penalty-active-p state)
    (decf (game-state-iron-penalty-timer state) (coerce dt 'single-float))
    (when (<= (game-state-iron-penalty-timer state) 0.0)
      (setf (game-state-iron-penalty-timer state) 0.0)
      (add-notification! state "The mine tunnels reopen." :color :text-dim)))

  ;; Event scheduling / triggering
  (when (null (game-state-active-event state))
    (when (<= (game-state-event-timer state) 0.0)
      ;; Back-compat for old saves / fresh starts.
      (setf (game-state-event-timer state) 90.0))
    (decf (game-state-event-timer state) (coerce dt 'single-float))
  (when (<= (game-state-event-timer state) 0.0)
      (start-random-event! state)))

  state)

(defevents
  (:cave-in "Cave-In!"
   "A tremor rocks the mine.\nDust fills the air and a tunnel collapses."
   (choice "Shore it up (cost: 50 stone)"
     :can (resource-has-p s :stone 50.0d0)
     (resource-sub s :stone 50.0d0)
     (add-notification! s "You reinforce the supports. Safe again." :color :green-ok))
   (choice "Abandon it (-10% iron for 60s)"
     (setf (game-state-iron-penalty-timer s) 60.0)
     (add-notification! s "Iron production reduced temporarily." :color :warn)))

  (:wandering-merchant "Wandering Merchant"
   "A hooded trader offers rare goods.\nHe prefers bars and alloyed wares."
   (choice "Trade 5 steel → 3 mythril"
     :can (resource-has-p s :steel 5.0d0)
     (resource-sub s :steel 5.0d0)
     (resource-add s :mythril 3.0d0)
     (add-notification! s "Trade complete." :color :green-ok))
   (choice "Trade 10 bronze → 500 coins"
     :can (resource-has-p s :bronze 10.0d0)
     (resource-sub s :bronze 10.0d0)
     (resource-add s :coins 500.0d0)
     (add-notification! s "Coins clink into your pouch." :color :gold))
   (choice "Send him away"))

  (:ember-pulse "Ember Pulse"
   "An ember crystal resonates.\nThe forge glows hotter for a moment."
   (choice "Harness the pulse (2× smelt speed, 30s)"
     (setf (game-state-smelt-boost-timer s)
           (max 30.0 (game-state-smelt-boost-timer s)))
     (add-notification! s "Smelting speed surged!" :color :red))
   (choice "Let it pass"))

  (:scrap-windfall "Scrap Windfall"
   "A broken caravan leaves behind a pile of scrap.\nNot all of it is useless."
   (choice "Salvage bars (+5 iron bars, +3 copper bars)"
     (resource-add s :iron-b 5.0d0)
     (resource-add s :copper-b 3.0d0)
     (add-notification! s "You salvage usable metal." :color :green-ok))
   (choice "Sell the scrap (+350 coins)"
     (resource-add s :coins 350.0d0)
     (add-notification! s "The market pays in full." :color :gold))
   (choice "Leave it"))

  (:ancient-cache "Ancient Cache"
   "You crack open a sealed coffer.\nInside: neatly packed components."
   (choice "Take the gears (+12 gear)"
     (resource-add s :gear 12.0d0)
     (add-notification! s "Gears clatter into your pack." :color :green-ok))
   (choice "Take the plates (+4 steel plates)"
     (resource-add s :steel-plate 4.0d0)
     (add-notification! s "Heavy plates, solid workmanship." :color :green-ok))
   (choice "Take the rivets (+60 rivets)"
     (resource-add s :rivet 60.0d0)
     (add-notification! s "A tin of rivets. Useful." :color :green-ok))))

(defstruct achievement-def
  (id :none :type keyword)
  (name "" :type string)
  (description "" :type string)
  (unlock nil)) ; NIL or (lambda (state) ...) -> boolean

(defachievements
  (:first-spark "First Spark"
   "Smelt your first bar."
   (or (>= (resource-ever s :iron-b) 1.0d0)
       (>= (resource-ever s :copper-b) 1.0d0)))

  (:iron-will "Iron Will"
   "Own 10 Iron Mines."
   (>= (building-count s :iron-mine) 10))

  (:the-alchemist "The Alchemist"
   "Discover the Bronze recipe."
   (>= (resource-ever s :bronze) 1.0d0))

  (:deep-echo "Deep Echo"
   "Extract your first Mythril."
   (>= (resource-ever s :mythril) 1.0d0))

  (:ember-awakened "Ember Awakened"
   "Condense your first Ember Crystal."
   (>= (resource-ever s :ember) 1.0d0))

  (:eternal-flame "Eternal Flame"
   "Ascend 5 times."
   (>= (game-state-ascensions s) 5))

  (:stone-stockpile "Stone Stockpile"
   "Produce 1,000 stone."
   (>= (resource-ever s :stone) 1000.0d0))

  (:coal-baron "Coal Baron"
   "Produce 500 coal."
   (>= (resource-ever s :coal) 500.0d0))

  (:pickaxe-army "Pickaxe Army"
   "Own 25 Pickaxes."
   (>= (building-count s :pickaxe) 25))

  (:furnace-fleet "Furnace Fleet"
   "Own 3 Auto-Furnaces."
   (>= (building-count s :furnace) 3))

  (:bellows-business "Bellows Business"
   "Own 2 Bellows Workshops."
   (>= (building-count s :bellows-shop) 2))

  (:merchant-prince "Merchant Prince"
   "Produce 10,000 coins."
   (>= (resource-ever s :coins) 10000.0d0))

  (:gearhead "Gearhead"
   "Produce 50 gears."
   (>= (resource-ever s :gear) 50.0d0))

  (:airflow-artisan "Airflow Artisan"
   "Craft 10 bellows."
   (>= (resource-ever s :bellows) 10.0d0))

  (:bronze-age "Bronze Age"
   "Produce 25 bronze bars."
   (>= (resource-ever s :bronze) 25.0d0))

  (:steelworks "Steelworks"
   "Produce 25 steel bars."
   (>= (resource-ever s :steel) 25.0d0))

  (:plate-press "Plate Press"
   "Hammer 10 steel plates."
   (>= (resource-ever s :steel-plate) 10.0d0))

  (:rivet-rain "Rivet Rain"
   "Forge 200 rivets."
   (>= (resource-ever s :rivet) 200.0d0))

  (:parts-bin "Parts Bin"
   "Assemble 3 machine parts."
   (>= (resource-ever s :machine-part) 3.0d0))

  (:shopaholic "Shopaholic"
   "Buy 5 shop perks total."
   (let ((sum 0))
     (maphash (lambda (k v) (declare (ignore k)) (incf sum v))
              (game-state-shop-purchases s))
     (>= sum 5)))

  (:queue-master "Queue Master"
   "Fill 8 smelt slots."
   (>= (length (game-state-smelt-queue s)) 8))

  (:tinkerer "Tinkerer"
   "Buy 10 upgrades."
   (>= (length (game-state-upgrades s)) 10))

  (:industrial-revolution "Industrial Revolution"
   "Buy Assembly Lines."
   (upgrade-owned-p s :assembly-lines)))

(defun achievement-unlocked-p (state id)
  (member id (game-state-achievements state)))

(defun unlock-achievement! (state id)
  (let ((def (find id *achievements* :key #'achievement-def-id :test #'eq)))
    (when (and def (not (achievement-unlocked-p state id)))
      (push id (game-state-achievements state))
      (add-notification! state (format nil "Achievement: ~A" (achievement-def-name def)) :color :gold)
      t)))

(defun maybe-unlock-achievements! (state)
  (dolist (def *achievements* state)
    (let ((id (achievement-def-id def))
          (u (achievement-def-unlock def)))
      (when (and u (not (achievement-unlocked-p state id)) (funcall u state))
        (unlock-achievement! state id)))))
