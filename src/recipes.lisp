(in-package #:ember-forge)

(defstruct recipe-def
  (id :none :type keyword)
  (name "" :type string)
  (inputs '() :type list)   ; plist (:resource amount ...)
  (outputs '() :type list)  ; plist (:resource amount ...)
  (duration 5.0 :type single-float)
  (unlock-condition nil))

(defparameter *recipes*
  (list
   (make-recipe-def
    :id :iron-bar
    :name "Smelt Iron Bar"
    :inputs '(:iron 3.0d0 :coal 1.0d0)
    :outputs '(:iron-b 1.0d0)
    :duration 4.0)

   (make-recipe-def
    :id :copper-bar
    :name "Smelt Copper Bar"
    :inputs '(:copper 3.0d0 :coal 1.0d0)
    :outputs '(:copper-b 1.0d0)
    :duration 4.0)

   (make-recipe-def
    :id :bronze
    :name "Alloy Bronze"
    :inputs '(:copper-b 2.0d0 :tin 1.0d0)
    :outputs '(:bronze 1.0d0)
    :duration 8.0
    :unlock-condition (lambda (s)
                        (and (resource-has-p s :copper-b 1.0d0)
                             (resource-has-p s :tin 1.0d0))))

   (make-recipe-def
    :id :steel
    :name "Forge Steel"
    :inputs '(:iron-b 2.0d0 :coal 3.0d0)
    :outputs '(:steel 1.0d0)
    :duration 12.0
    :unlock-condition (lambda (s) (upgrade-owned-p s :steel-alloy)))

   (make-recipe-def
    :id :gear
    :name "Cast Gear"
    :inputs '(:iron-b 1.0d0)
    :outputs '(:gear 1.0d0)
    :duration 3.0)

   (make-recipe-def
    :id :bellows-part
    :name "Craft Bellows"
    :inputs '(:bronze 2.0d0 :coal 2.0d0)
    :outputs '(:bellows 1.0d0)
    :duration 10.0)

   (make-recipe-def
    :id :machined-gears
    :name "Machine Gears"
    :inputs '(:steel 1.0d0)
    :outputs '(:gear 3.0d0)
    :duration 6.0
    :unlock-condition (lambda (s) (>= (resource-ever s :steel) 1.0d0)))

   (make-recipe-def
    :id :rivets
    :name "Forge Rivets"
    :inputs '(:iron-b 1.0d0 :coal 1.0d0)
    :outputs '(:rivet 8.0d0)
    :duration 6.0
    :unlock-condition (lambda (s) (upgrade-owned-p s :metalworking)))

   (make-recipe-def
    :id :steel-plate
    :name "Hammer Steel Plate"
    :inputs '(:steel 2.0d0 :coal 1.0d0)
    :outputs '(:steel-plate 1.0d0)
    :duration 14.0
    :unlock-condition (lambda (s) (upgrade-owned-p s :metalworking)))

   (make-recipe-def
    :id :machine-part
    :name "Assemble Machine Part"
    :inputs '(:gear 2.0d0 :steel-plate 1.0d0 :rivet 10.0d0)
    :outputs '(:machine-part 1.0d0)
    :duration 22.0
    :unlock-condition (lambda (s) (upgrade-owned-p s :industrial-tools)))

   (make-recipe-def
    :id :coin-mint
    :name "Mint Trade Tokens"
    :inputs '(:bronze 1.0d0 :gear 1.0d0)
    :outputs '(:coins 250.0d0)
    :duration 10.0
    :unlock-condition (lambda (s) (upgrade-owned-p s :coin-minting)))

   (make-recipe-def
    :id :ember-crystal
    :name "Condense Ember Crystal"
    :inputs '(:mythril 10.0d0 :steel 5.0d0)
    :outputs '(:ember 1.0d0)
    :duration 30.0
    :unlock-condition (lambda (s) (>= (building-count s :mythril-drill) 1)))))

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
