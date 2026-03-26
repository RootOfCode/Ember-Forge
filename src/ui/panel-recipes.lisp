(in-package #:ember-forge)

(defun %fmt-io (plist)
  (with-output-to-string (s)
    (loop for (res amt) on plist by #'cddr
          for i from 0
          do (progn
               (when (> i 0) (write-string ", " s))
               (format s "~A ~A" (fmt-number amt) (resource-name res))))))

(defun render-panel-recipes (ren state font font-lg x y w h)
  (draw-panel-background ren :recipes x y w h)
  (draw-label ren font-lg "RECIPES" (+ x 20) (+ y 10) :text)

  (let* ((row-h 54)
         (r-x (+ x 24))
         (r-y (+ y 56))
         (r-w (- w 48))
         (col-gap 16)
         (col-w (max 0 (floor (/ (- r-w col-gap) 2))))
         (queue-room (< (length (game-state-smelt-queue state)) (max-smelt-slots state)))
         (items (loop for def in *recipes*
                      when (recipe-available-p state def)
                        collect def)))
    (loop for def in items
          for idx from 0
          for col = (mod idx 2)
          for row = (floor idx 2)
          for col-x = (+ r-x (* col (+ col-w col-gap)))
          for row-y = (+ r-y (* row (+ row-h 10)))
          while (< (+ row-y row-h) (+ y h -8))
          do (let* ((can-buy (and queue-room (can-afford-recipe-p state def)))
                    (btn-x (+ col-x (- col-w 92)))
                    (btn-y (+ row-y 10))
                    (btn-w 84)
                    (btn-h (- row-h 20))
                    (text-x (+ col-x 10))
                    (text-right (- btn-x 12))
                    (text-w (max 0 (- text-right text-x))))
               (draw-rect ren col-x row-y col-w row-h :bg-sidebar :border-color :btn-hover)
               (draw-label-fit ren font (recipe-def-name def) text-x (+ row-y 6) :text (max 0 (- text-w 70)))
               (draw-label-fit ren font (format nil "In: ~A" (%fmt-io (recipe-def-inputs def)))
                               text-x (+ row-y 24) :text-dim text-w)
               (draw-label-fit ren font (format nil "Out: ~A" (%fmt-io (recipe-def-outputs def)))
                               text-x (+ row-y 40) :text-dim text-w)
               (draw-label ren font (format nil "~,1Fs" (recipe-def-duration def))
                           text-right (+ row-y 6) :text-dim :align :right)
               (when (draw-button ren font "Smelt"
                                  btn-x btn-y btn-w btn-h
                                  :enabled can-buy
                                  :tooltip "Queue this recipe into the smelting queue.")
                 (enqueue-smelt-job! state (recipe-def-id def) :source :manual)))))
  state)
