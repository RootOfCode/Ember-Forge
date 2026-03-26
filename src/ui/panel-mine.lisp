(in-package #:ember-forge)

(defun render-panel-mine (ren state font font-lg x y w h)
  (draw-panel-background ren :mine x y w h)
  (draw-label ren font-lg "MINE" (+ x 20) (+ y 10) :text)

  (let* ((rows (sort (copy-list (game-state-unlocked state))
                     (lambda (a b)
                       (let ((ta (resource-tier a))
                             (tb (resource-tier b)))
                         (if (= ta tb)
                             (string< (resource-name a) (resource-name b))
                             (< ta tb))))))
         (row-h 34)
         (r-x (+ x 24))
         (r-y (+ y 56))
         (r-w (- w 48))
         (gather-job (game-state-manual-gather state)))
    (loop for res in rows
          for i from 0
          for y0 = (+ r-y (* i (+ row-h 6)))
          for amt = (resource-amount state res)
          for cap = (resource-cap state res)
          for rate = (gethash res (game-state-production state) 0.0d0)
          do (draw-rect ren r-x y0 r-w row-h :bg-sidebar :border-color :btn-hover)
             (draw-resource-icon ren res (+ r-x 10) (+ y0 10))
             (draw-label ren font (resource-name res) (+ r-x 30) (+ y0 6) :text)
             (draw-label ren font (format nil "~A / ~A" (fmt-number amt) (fmt-number cap))
                         (+ r-x 260) (+ y0 6) :text-dim)
             (draw-label ren font (fmt-rate rate) (+ r-x 460) (+ y0 6) :text-dim)
             (when (and (not (eq res :coins)) (> (sell-value res) 0.0d0))
               (when (draw-button ren font "Sell 10"
                                  (+ r-x (- r-w 280)) (+ y0 4) 120 (- row-h 8)
                                  :enabled (can-sell-p state res 10.0d0)
                                  :tooltip (format nil "Sell 10 ~A (value: ~A coins)."
                                                   (resource-name res)
                                                   (fmt-number (* 10.0d0 (sell-value res) (game-state-sell-mult state)))))
                 (sell-resource! state res 10.0d0)))
             (when (= (resource-tier res) 0)
               (let* ((this (and gather-job (eq (manual-job-resource gather-job) res)))
                      (enabled (null gather-job))
                      (label (cond
                               (this (format nil "Gathering ~,1Fs" (manual-job-timer gather-job)))
                               (gather-job "Busy")
                               (t "Gather"))))
                 (when (draw-button ren font label
                                    (+ r-x (- r-w 140)) (+ y0 4) 132 (- row-h 8)
                                    :enabled enabled
                                    :tooltip (format nil "Gather ~A (takes ~,1Fs)."
                                                     (resource-name res)
                                                     (game-state-manual-gather-duration state)))
                   (start-manual-gather! state res))
                 (when this
                   (draw-progress-bar ren (+ r-x (- r-w 470)) (+ y0 10) 168 12
                                      (manual-job-progress gather-job)
                                      :gold :bg-panel))))))
  state)
