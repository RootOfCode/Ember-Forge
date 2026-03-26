(in-package #:ember-forge)

(defun %fmt-cost (plist)
  (with-output-to-string (s)
    (loop for (res amt) on plist by #'cddr
          for i from 0
          do (progn
               (when (> i 0) (write-string ", " s))
               (format s "~A ~A" (fmt-number amt) (resource-name res))))))

(defun render-panel-forge (ren state font font-lg x y w h)
  (draw-panel-background ren :forge x y w h)
  (draw-label ren font-lg "FORGE" (+ x 20) (+ y 10) :text)

  ;; Click-to-mine
  (let* ((mine-job (game-state-manual-mine state))
         (btn-w 360)
         (btn-h 44)
         (btn-x (+ x (floor (- w btn-w) 2)))
         (btn-y (+ y 48)))
    (when (draw-button ren font
                       (if mine-job
                           (format nil "MINING... ~,1Fs" (manual-job-timer mine-job))
                           "CLICK TO MINE")
                       btn-x btn-y btn-w btn-h
                       :enabled (null mine-job)
                       :tooltip (if mine-job
                                    (format nil "Mining... ~,1Fs remaining." (manual-job-timer mine-job))
                                    (format nil "Mine stone (takes ~,1Fs)." (game-state-manual-mine-duration state))))
      (start-manual-mine! state))
    (when mine-job
      (draw-progress-bar ren btn-x (+ btn-y btn-h 6) btn-w 10
                         (manual-job-progress mine-job)
                         :gold :bg-sidebar)))
  (draw-label ren font
              (format nil "Click power: ~A stone" (fmt-number (game-state-click-mult state)))
              (+ x 24) (+ y 100) :text-dim)

  ;; Smelt queue
  (let* ((q-x (+ x 24))
         (q-y (+ y 130))
         (q-w (- w 48))
         (row-h 28)
         (max (max-smelt-slots state))
         (shown (min max 6))
         (queue (game-state-smelt-queue state)))
    (draw-label ren font "SMELTING QUEUE" q-x (- q-y 18) :text)
    (loop for i from 0 below shown
          for row-y = (+ q-y (* i (+ row-h 6)))
          for job = (nth i queue)
          do (draw-rect ren q-x row-y q-w row-h :bg-sidebar :border-color :btn-hover)
             (if job
                 (let* ((def (find-recipe-def (smelt-job-recipe job)))
                        (name (if def (recipe-def-name def) "???"))
                        (p (smelt-job-progress job)))
                   (draw-label ren font name (+ q-x 10) (+ row-y 6) :text)
                   (draw-progress-bar ren (+ q-x 220) (+ row-y 8) 220 12 p :gold :bg-panel)
                   (when (draw-button ren font "Cancel" (+ q-x (- q-w 80)) (+ row-y 2) 72 (- row-h 4)
                                      :enabled t)
                     (cancel-smelt-job! state i)))
                 (draw-label ren font (if (< i max) "(empty)" "(locked)")
                             (+ q-x 10) (+ row-y 6) :text-dim))))

  ;; Buildings
  (let* ((b-x (+ x 24))
         (b-y (+ y 340))
         (b-w (- w 48))
         (row-h 34)
         (i 0))
    (draw-label ren font "BUILDINGS" b-x (- b-y 18) :text)
    (dolist (def *buildings*)
      (when (building-unlocked-p state def)
        (let* ((id (building-def-id def))
               (count (building-count state id))
               (owned count)
               (cost (building-cost def owned))
               (can-buy (can-afford-plist-p state cost))
               (row-y (+ b-y (* i (+ row-h 6)))))
          (incf i)
          (draw-rect ren b-x row-y b-w row-h :bg-sidebar :border-color :btn-hover)
          (draw-label ren font
                      (format nil "~A  ×~D" (building-def-name def) count)
                      (+ b-x 10) (+ row-y 6) :text)
          (draw-label ren font
                      (if (building-def-production def)
                          (with-output-to-string (s)
                            (loop for (res rate) on (building-def-production def) by #'cddr
                                  for j from 0
                                  do (progn
                                       (when (> j 0) (write-string ", " s))
                                       (format s "+~A ~A"
                                               (fmt-rate (* rate (building-prod-mult state id)))
                                               (resource-name res)))))
                          "auto-smelt iron bars")
                      (+ b-x 320) (+ row-y 6) :text-dim)
          (draw-label ren font (%fmt-cost cost)
                      (+ b-x (- b-w 96)) (+ row-y 6) :text-dim :align :right)
          (when (draw-button ren font "Buy" (+ b-x (- b-w 86)) (+ row-y 4) 78 (- row-h 8)
                             :enabled can-buy)
            (buy-building! state id))))))

  state)
