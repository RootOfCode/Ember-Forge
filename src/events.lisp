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

(defparameter *events*
  (list
   (make-event-def
    :id :cave-in
    :name "Cave-In!"
    :text "A tremor rocks the mine.\nDust fills the air and a tunnel collapses."
    :choices (list
              (%event-choice
               "Shore it up (cost: 50 stone)"
               (lambda (s)
                 (resource-sub s :stone 50.0d0)
                 (add-notification! s "You reinforce the supports. Safe again." :color :green-ok))
               :can (lambda (s) (resource-has-p s :stone 50.0d0)))
              (%event-choice
               "Abandon it (-10% iron for 60s)"
               (lambda (s)
                 (setf (game-state-iron-penalty-timer s) 60.0)
                 (add-notification! s "Iron production reduced temporarily." :color :warn)))))

   (make-event-def
    :id :wandering-merchant
    :name "Wandering Merchant"
    :text "A hooded trader offers rare goods.\nHe prefers bars and alloyed wares."
    :choices (list
              (%event-choice
               "Trade 5 steel → 3 mythril"
               (lambda (s)
                 (resource-sub s :steel 5.0d0)
                 (resource-add s :mythril 3.0d0)
                 (add-notification! s "Trade complete." :color :green-ok))
               :can (lambda (s) (resource-has-p s :steel 5.0d0)))
              (%event-choice
               "Trade 10 bronze → 500 coins"
               (lambda (s)
                 (resource-sub s :bronze 10.0d0)
                 (resource-add s :coins 500.0d0)
                 (add-notification! s "Coins clink into your pouch." :color :gold))
               :can (lambda (s) (resource-has-p s :bronze 10.0d0)))
              (%event-choice
               "Send him away"
               (lambda (s)
                 (declare (ignore s))))))

   (make-event-def
    :id :ember-pulse
    :name "Ember Pulse"
    :text "An ember crystal resonates.\nThe forge glows hotter for a moment."
    :choices (list
              (%event-choice
               "Harness the pulse (2× smelt speed, 30s)"
               (lambda (s)
                 (setf (game-state-smelt-boost-timer s)
                       (max 30.0 (game-state-smelt-boost-timer s)))
                 (add-notification! s "Smelting speed surged!" :color :red)))
              (%event-choice
               "Let it pass"
               (lambda (s)
                 (declare (ignore s))))))

   (make-event-def
    :id :scrap-windfall
    :name "Scrap Windfall"
    :text "A broken caravan leaves behind a pile of scrap.\nNot all of it is useless."
    :choices (list
              (%event-choice
               "Salvage bars (+5 iron bars, +3 copper bars)"
               (lambda (s)
                 (resource-add s :iron-b 5.0d0)
                 (resource-add s :copper-b 3.0d0)
                 (add-notification! s "You salvage usable metal." :color :green-ok)))
              (%event-choice
               "Sell the scrap (+350 coins)"
               (lambda (s)
                 (resource-add s :coins 350.0d0)
                 (add-notification! s "The market pays in full." :color :gold)))
              (%event-choice
               "Leave it"
               (lambda (s)
                 (declare (ignore s))))))

   (make-event-def
    :id :ancient-cache
    :name "Ancient Cache"
    :text "You crack open a sealed coffer.\nInside: neatly packed components."
    :choices (list
              (%event-choice
               "Take the gears (+12 gear)"
               (lambda (s)
                 (resource-add s :gear 12.0d0)
                 (add-notification! s "Gears clatter into your pack." :color :green-ok)))
              (%event-choice
               "Take the plates (+4 steel plates)"
               (lambda (s)
                 (resource-add s :steel-plate 4.0d0)
                 (add-notification! s "Heavy plates, solid workmanship." :color :green-ok)))
              (%event-choice
               "Take the rivets (+60 rivets)"
               (lambda (s)
                 (resource-add s :rivet 60.0d0)
                 (add-notification! s "A tin of rivets. Useful." :color :green-ok)))))))

(defstruct achievement-def
  (id :none :type keyword)
  (name "" :type string)
  (description "" :type string)
  (unlock nil)) ; NIL or (lambda (state) ...) -> boolean

(defparameter *achievements*
  (list
   (make-achievement-def
    :id :first-spark
    :name "First Spark"
    :description "Smelt your first bar."
    :unlock (lambda (s)
              (or (>= (resource-ever s :iron-b) 1.0d0)
                  (>= (resource-ever s :copper-b) 1.0d0))))
   (make-achievement-def
    :id :iron-will
    :name "Iron Will"
    :description "Own 10 Iron Mines."
    :unlock (lambda (s) (>= (building-count s :iron-mine) 10)))
   (make-achievement-def
    :id :the-alchemist
    :name "The Alchemist"
    :description "Discover the Bronze recipe."
    :unlock (lambda (s) (>= (resource-ever s :bronze) 1.0d0)))
   (make-achievement-def
    :id :deep-echo
    :name "Deep Echo"
    :description "Extract your first Mythril."
    :unlock (lambda (s) (>= (resource-ever s :mythril) 1.0d0)))
   (make-achievement-def
    :id :ember-awakened
    :name "Ember Awakened"
    :description "Condense your first Ember Crystal."
    :unlock (lambda (s) (>= (resource-ever s :ember) 1.0d0)))
   (make-achievement-def
    :id :eternal-flame
    :name "Eternal Flame"
    :description "Ascend 5 times."
    :unlock (lambda (s) (>= (game-state-ascensions s) 5)))

   (make-achievement-def
    :id :stone-stockpile
    :name "Stone Stockpile"
    :description "Produce 1,000 stone."
    :unlock (lambda (s) (>= (resource-ever s :stone) 1000.0d0)))

   (make-achievement-def
    :id :coal-baron
    :name "Coal Baron"
    :description "Produce 500 coal."
    :unlock (lambda (s) (>= (resource-ever s :coal) 500.0d0)))

   (make-achievement-def
    :id :pickaxe-army
    :name "Pickaxe Army"
    :description "Own 25 Pickaxes."
    :unlock (lambda (s) (>= (building-count s :pickaxe) 25)))

   (make-achievement-def
    :id :furnace-fleet
    :name "Furnace Fleet"
    :description "Own 3 Auto-Furnaces."
    :unlock (lambda (s) (>= (building-count s :furnace) 3)))

   (make-achievement-def
    :id :bellows-business
    :name "Bellows Business"
    :description "Own 2 Bellows Workshops."
    :unlock (lambda (s) (>= (building-count s :bellows-shop) 2)))

   (make-achievement-def
    :id :merchant-prince
    :name "Merchant Prince"
    :description "Produce 10,000 coins."
    :unlock (lambda (s) (>= (resource-ever s :coins) 10000.0d0)))

   (make-achievement-def
    :id :gearhead
    :name "Gearhead"
    :description "Produce 50 gears."
    :unlock (lambda (s) (>= (resource-ever s :gear) 50.0d0)))

   (make-achievement-def
    :id :airflow-artisan
    :name "Airflow Artisan"
    :description "Craft 10 bellows."
    :unlock (lambda (s) (>= (resource-ever s :bellows) 10.0d0)))

   (make-achievement-def
    :id :bronze-age
    :name "Bronze Age"
    :description "Produce 25 bronze bars."
    :unlock (lambda (s) (>= (resource-ever s :bronze) 25.0d0)))

   (make-achievement-def
    :id :steelworks
    :name "Steelworks"
    :description "Produce 25 steel bars."
    :unlock (lambda (s) (>= (resource-ever s :steel) 25.0d0)))

   (make-achievement-def
    :id :plate-press
    :name "Plate Press"
    :description "Hammer 10 steel plates."
    :unlock (lambda (s) (>= (resource-ever s :steel-plate) 10.0d0)))

   (make-achievement-def
    :id :rivet-rain
    :name "Rivet Rain"
    :description "Forge 200 rivets."
    :unlock (lambda (s) (>= (resource-ever s :rivet) 200.0d0)))

   (make-achievement-def
    :id :parts-bin
    :name "Parts Bin"
    :description "Assemble 3 machine parts."
    :unlock (lambda (s) (>= (resource-ever s :machine-part) 3.0d0)))

   (make-achievement-def
    :id :shopaholic
    :name "Shopaholic"
    :description "Buy 5 shop perks total."
    :unlock (lambda (s)
              (let ((sum 0))
                (maphash (lambda (k v) (declare (ignore k)) (incf sum v))
                         (game-state-shop-purchases s))
                (>= sum 5))))

   (make-achievement-def
    :id :queue-master
    :name "Queue Master"
    :description "Fill 8 smelt slots."
    :unlock (lambda (s) (>= (length (game-state-smelt-queue s)) 8)))

   (make-achievement-def
    :id :tinkerer
    :name "Tinkerer"
    :description "Buy 10 upgrades."
    :unlock (lambda (s) (>= (length (game-state-upgrades s)) 10)))

   (make-achievement-def
    :id :industrial-revolution
    :name "Industrial Revolution"
    :description "Buy Assembly Lines."
    :unlock (lambda (s) (upgrade-owned-p s :assembly-lines)))))

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
