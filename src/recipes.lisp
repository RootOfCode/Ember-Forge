(in-package #:ember-forge)

(defstruct recipe-def
  (id :none :type keyword)
  (name "" :type string)
  (inputs '() :type list)   ; plist (:resource amount ...)
  (outputs '() :type list)  ; plist (:resource amount ...)
  (duration 5.0 :type single-float)
  (unlock-condition nil))

(defrecipes
  (:iron-bar "Smelt Iron Bar"
   :in (:iron 3.0d0 :coal 1.0d0)
   :out (:iron-b 1.0d0)
   :time 4.0)

  (:copper-bar "Smelt Copper Bar"
   :in (:copper 3.0d0 :coal 1.0d0)
   :out (:copper-b 1.0d0)
   :time 4.0)

  (:bronze "Alloy Bronze"
   :in (:copper-b 2.0d0 :tin 1.0d0)
   :out (:bronze 1.0d0)
   :time 8.0
   :unlock (and (resource-has-p s :copper-b 1.0d0)
                (resource-has-p s :tin 1.0d0)))

  (:steel "Forge Steel"
   :in (:iron-b 2.0d0 :coal 3.0d0)
   :out (:steel 1.0d0)
   :time 12.0
   :unlock (upgrade-owned-p s :steel-alloy))

  (:gear "Cast Gear"
   :in (:iron-b 1.0d0)
   :out (:gear 1.0d0)
   :time 3.0)

  (:bellows-part "Craft Bellows"
   :in (:bronze 2.0d0 :coal 2.0d0)
   :out (:bellows 1.0d0)
   :time 10.0)

  (:machined-gears "Machine Gears"
   :in (:steel 1.0d0)
   :out (:gear 3.0d0)
   :time 6.0
   :unlock (>= (resource-ever s :steel) 1.0d0))

  (:rivets "Forge Rivets"
   :in (:iron-b 1.0d0 :coal 1.0d0)
   :out (:rivet 8.0d0)
   :time 6.0
   :unlock (upgrade-owned-p s :metalworking))

  (:steel-plate "Hammer Steel Plate"
   :in (:steel 2.0d0 :coal 1.0d0)
   :out (:steel-plate 1.0d0)
   :time 14.0
   :unlock (upgrade-owned-p s :metalworking))

  (:machine-part "Assemble Machine Part"
   :in (:gear 2.0d0 :steel-plate 1.0d0 :rivet 10.0d0)
   :out (:machine-part 1.0d0)
   :time 22.0
   :unlock (upgrade-owned-p s :industrial-tools))

  (:coin-mint "Mint Trade Tokens"
   :in (:bronze 1.0d0 :gear 1.0d0)
   :out (:coins 250.0d0)
   :time 10.0
   :unlock (upgrade-owned-p s :coin-minting))

  (:ember-crystal "Condense Ember Crystal"
   :in (:mythril 10.0d0 :steel 5.0d0)
   :out (:ember 1.0d0)
   :time 30.0
   :unlock (>= (building-count s :mythril-drill) 1)))

(defun find-recipe-def (id)
  (find id *recipes* :key #'recipe-def-id :test #'eq))

(defun recipe-available-p (state def)
  (let ((u (recipe-def-unlock-condition def)))
    (if u (funcall u state) t)))

(defun can-afford-recipe-p (state def)
  (can-afford-plist-p state (recipe-def-inputs def)))

(defun enqueue-smelt-job! (state recipe-id &key (source :manual))
  (let ((def (find-recipe-def recipe-id)))
    (when (and def
               (recipe-available-p state def)
               (< (length (game-state-smelt-queue state)) (max-smelt-slots state))
               (can-afford-recipe-p state def))
      (spend-plist! state (recipe-def-inputs def))
      (push (make-smelt-job :recipe recipe-id
                            :progress 0.0
                            :duration (coerce (recipe-def-duration def) 'single-float)
                            :source source)
            (game-state-smelt-queue state))
      (add-notification! state (format nil "Queued: ~A" (recipe-def-name def)) :color :text)
      t)))

(defun cancel-smelt-job! (state idx)
  (let* ((queue (game-state-smelt-queue state))
         (job (nth idx queue)))
    (when job
      (let ((def (find-recipe-def (smelt-job-recipe job))))
        (when def
          ;; Full refund on cancel (simple, forgiving).
          (loop for (res amt) on (recipe-def-inputs def) by #'cddr
                do (resource-add state res amt))
          (add-notification! state "Canceled smelt job" :color :warn)))
      (setf (game-state-smelt-queue state)
            (loop for j in queue
                  for i from 0
                  unless (= i idx) collect j))
      t)))

(defun tick-smelt-queue! (state dt)
  (let* ((speed (smelt-speed-mult state))
         (done '())
         (newq '()))
    (dolist (job (game-state-smelt-queue state))
      (let* ((dur (max 0.01 (smelt-job-duration job)))
             (p (+ (smelt-job-progress job)
                   (/ (* (coerce dt 'single-float) (coerce speed 'single-float)) dur))))
        (setf (smelt-job-progress job) p)
        (if (>= p 1.0)
            (push job done)
            (push job newq))))
    (setf (game-state-smelt-queue state) (nreverse newq))
    (dolist (job done)
      (let ((def (find-recipe-def (smelt-job-recipe job))))
        (when def
          (let* ((rid (recipe-def-id def))
                 (mult (cond
                         ((and (member rid '(:gear :bellows-part :rivets))
                               (upgrade-owned-p state :precision-molds))
                          2.0d0)
                         (t 1.0d0))))
            (loop for (res amt) on (recipe-def-outputs def) by #'cddr
                  do (resource-add state res (* (coerce amt 'double-float) mult)))
            (add-notification! state (format nil "Completed: ~A" (recipe-def-name def))
                               :color :green-ok))))))
  state)

(defun tick-auto-furnaces! (state)
  (let* ((desired (building-count state :furnace))
         (running (count-if (lambda (j)
                              (and (eq (smelt-job-source j) :auto)
                                   (eq (smelt-job-recipe j) :iron-bar)))
                            (game-state-smelt-queue state))))
    (loop while (and (< running desired)
                     (< (length (game-state-smelt-queue state)) (max-smelt-slots state)))
          do (if (enqueue-smelt-job! state :iron-bar :source :auto)
                 (incf running)
                 (return))))
  state)
