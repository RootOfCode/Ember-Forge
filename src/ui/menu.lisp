(in-package #:ember-forge)

(defun render-main-menu (ren state font font-lg)
  (draw-panel-background ren :menu 0 0 +window-width+ +window-height+ :overlay '(0 0 0 190))

  (let* ((cx (floor (/ +window-width+ 2)))
         (title-y 120))
    (draw-label ren font-lg "EMBER FORGE" cx title-y :text :align :center)
    (draw-label ren font "mine → smelt → build → ascend" cx (+ title-y 30) :text-dim :align :center)

    (let* ((box-w 460)
           (btn-h 40)
           (btn-gap 12)
           (num-buttons (if (game-state-game-started-p state) 5 4))
           (box-h (+ 28 (* num-buttons btn-h) (* (1- num-buttons) btn-gap) 28))
           (box-x (floor (/ (- +window-width+ box-w) 2)))
           (box-y (+ title-y 70))
           (btn-w (- box-w 48))
           (btn-x (+ box-x 24))
           (btn-y (+ box-y 28))
           (row-step (+ btn-h btn-gap)))
      (draw-rect ren box-x box-y box-w box-h :bg-panel :border-color :btn-active)

      (cond
        ((game-state-game-started-p state)
         (when (draw-button ren font "Resume" btn-x btn-y btn-w btn-h
                            :tooltip "Return to the forge.")
           (setf (game-state-screen state) :game))
         (when (draw-button ren font "Save" btn-x (+ btn-y row-step) btn-w btn-h
                            :tooltip "Write the save now.")
           (save-game state)
           (setf (game-state-last-save-ut state) (get-universal-time))
           (add-notification! state "Saved." :color :green-ok))
         (when (draw-button ren font "Options" btn-x (+ btn-y (* 2 row-step)) btn-w btn-h
                            :tooltip "Saves, autosave, and other settings.")
           (setf (game-state-screen state) :options))
         (when (draw-button ren font "Return to Title" btn-x (+ btn-y (* 3 row-step)) btn-w btn-h
                            :tooltip "Go back to the title screen.")
           (setf (game-state-game-started-p state) nil))
         (when (draw-button ren font "Quit" btn-x (+ btn-y (* 4 row-step)) btn-w btn-h)
           (sdl2:push-event :quit)))

        (t
         (let ((has-save (not (null (save-exists-p state)))))
           (when (draw-button ren font "Continue" btn-x btn-y btn-w btn-h
                              :enabled has-save
                              :tooltip (if has-save "Load your save." "No save found."))
             (start-continue-game! state))
           (when (draw-button ren font "New Game" btn-x (+ btn-y row-step) btn-w btn-h
                              :tooltip "Start fresh (does not delete files until you save).")
             (start-new-game! state))
           (when (draw-button ren font "Options" btn-x (+ btn-y (* 2 row-step)) btn-w btn-h
                              :tooltip "Saves, autosave, and other settings.")
             (setf (game-state-screen state) :options))
           (when (draw-button ren font "Quit" btn-x (+ btn-y (* 3 row-step)) btn-w btn-h)
             (sdl2:push-event :quit)))))))
  state)

(defun render-options-menu (ren state font font-lg)
  (draw-panel-background ren :menu 0 0 +window-width+ +window-height+ :overlay '(0 0 0 190))

  (let* ((cx (floor (/ +window-width+ 2)))
         (title-y 86))
    (draw-label ren font-lg "OPTIONS" cx title-y :text :align :center)
    (draw-label ren font "Saves, autosave, and gameplay settings." cx (+ title-y 30) :text-dim :align :center)

    (let* ((box-w 920)
           (box-h 560)
           (box-x (floor (/ (- +window-width+ box-w) 2)))
           (box-y (+ title-y 70)))
      (draw-rect ren box-x box-y box-w box-h :bg-panel :border-color :btn-active)

      (let* ((x0 (+ box-x 24))
             (yy (+ box-y 24))
             (row-h 38)
             (label-w 140)
             (btn-h 30)
             (btn-gap 10))
        ;; Save
        (draw-label ren font "SAVE" x0 yy :text)
        (incf yy 26)

        ;; Slot
        (draw-label ren font "Slot:" x0 (+ yy 6) :text-dim)
        (let* ((bx (+ x0 label-w)))
          (loop for s from 1 to 3
                for xx = (+ bx (* (1- s) 86))
                for exists = (save-exists-p state :slot s)
                for label = (if exists (format nil "~D*" s) (format nil "~D" s))
                do (when (draw-button ren font label xx yy 76 btn-h
                                      :color (if (= (game-state-save-slot state) s) :btn-active :btn-normal)
                                      :tooltip (if exists "Save found in this slot." "Empty slot."))
                     (setf (game-state-save-slot state) s)
                     (save-options state))))
        (incf yy row-h)

        ;; Paths
        (draw-label ren font "Save path:" x0 (+ yy 6) :text-dim)
        (draw-label-fit ren font (namestring (save-path state)) (+ x0 label-w) (+ yy 6) :text
                        (- box-w label-w 56))
        (incf yy 22)
        (draw-label ren font "Options:" x0 (+ yy 6) :text-dim)
        (draw-label-fit ren font (namestring (options-path)) (+ x0 label-w) (+ yy 6) :text
                        (- box-w label-w 56))
        (incf yy (+ row-h 6))

        ;; Actions
        (let* ((bx x0)
               (by yy)
               (btn-w 170)
               (has-save (save-exists-p state :slot (game-state-save-slot state)))
               (can-load (and (not (game-state-game-started-p state)) has-save))
               (can-save (game-state-game-started-p state))
               (can-delete (and (not (game-state-game-started-p state)) has-save)))
          (when (draw-button ren font "Save Now" bx by btn-w btn-h
                             :enabled can-save
                             :tooltip (if can-save "Write save immediately." "Start a game to save."))
            (save-game state)
            (setf (game-state-last-save-ut state) (get-universal-time))
            (add-notification! state "Saved." :color :green-ok))
          (when (draw-button ren font "Load Slot" (+ bx (+ btn-w btn-gap)) by btn-w btn-h
                             :enabled can-load
                             :tooltip (cond
                                        ((game-state-game-started-p state) "Return to title to load.")
                                        (has-save "Load this slot and start playing.")
                                        (t "No save in this slot.")))
            (start-continue-game! state))
          (when (draw-button ren font "Delete Slot" (+ bx (* 2 (+ btn-w btn-gap))) by btn-w btn-h
                             :enabled can-delete
                             :tooltip (cond
                                        ((game-state-game-started-p state) "Return to title to delete saves.")
                                        (has-save "Delete this slot's save.")
                                        (t "No save in this slot.")))
            (when (delete-save state :slot (game-state-save-slot state))
              (add-notification! state "Deleted save." :color :text-dim))))
        (incf yy (+ row-h 30))

        ;; Autosave
        (draw-label ren font "GAMEPLAY" x0 yy :text)
        (incf yy 26)
        (draw-label ren font "Autosave:" x0 (+ yy 6) :text-dim)
        (let* ((bx (+ x0 label-w))
               (enabled (game-state-autosave-enabled state)))
          (when (draw-button ren font (if enabled "On" "Off") bx yy 80 btn-h
                             :color (if enabled :btn-active :btn-normal)
                             :tooltip "Autosave while playing.")
            (setf (game-state-autosave-enabled state) (not enabled))
            (save-options state)))
        (incf yy row-h)
        (draw-label ren font "Interval:" x0 (+ yy 6) :text-dim)
        (let* ((bx (+ x0 label-w))
               (enabled (game-state-autosave-enabled state))
               (choices '(30 60 120)))
          (loop for n in choices
                for i from 0
                for xx = (+ bx (* i 86))
                do (when (draw-button ren font (format nil "~Ds" n) xx yy 76 btn-h
                                      :enabled enabled
                                      :color (if (= (game-state-autosave-interval state) n)
                                                 :btn-active
                                                 :btn-normal)
                                      :tooltip (if enabled "Autosave interval." "Enable autosave first."))
                     (setf (game-state-autosave-interval state) n)
                     (save-options state))))
        (incf yy row-h)

        ;; Offline progress
        (draw-label ren font "Offline:" x0 (+ yy 6) :text-dim)
        (let* ((bx (+ x0 label-w))
               (enabled (game-state-offline-progress-enabled state)))
          (when (draw-button ren font (if enabled "On" "Off") bx yy 80 btn-h
                             :color (if enabled :btn-active :btn-normal)
                             :tooltip "Apply offline production when loading a save.")
            (setf (game-state-offline-progress-enabled state) (not enabled))
            (save-options state)))
        (incf yy row-h)

        ;; Fullscreen
        (draw-label ren font "Fullscreen:" x0 (+ yy 6) :text-dim)
        (let* ((bx (+ x0 label-w))
               (enabled (game-state-fullscreen-enabled state)))
          (when (draw-button ren font (if enabled "On" "Off") bx yy 80 btn-h
                             :color (if enabled :btn-active :btn-normal)
                             :tooltip "Toggle fullscreen (F11).")
            (setf (game-state-fullscreen-enabled state) (not enabled))
            (save-options state)))
        (incf yy row-h)

        ;; Font
        (draw-label ren font "Font:" x0 (+ yy 6) :text-dim)
        (let* ((fonts (available-fonts))
               (n (length fonts))
               (idx (or (position (game-state-font-path state) fonts :test #'string=) 0))
               (label (if (> n 0)
                          (font-display-name (nth idx fonts))
                          "No fonts found"))
               (bx (+ x0 label-w))
               (btn-w 34)
               (btn-gap 8))
          (when (draw-button ren font "<" bx yy btn-w btn-h
                             :enabled (> n 1)
                             :tooltip "Previous font.")
            (let ((new (nth (mod (+ idx -1) n) fonts)))
              (setf (game-state-font-path state) new)
              (save-options state)))
          (when (draw-button ren font ">" (+ bx (+ btn-w btn-gap)) yy btn-w btn-h
                             :enabled (> n 1)
                             :tooltip "Next font.")
            (let ((new (nth (mod (+ idx 1) n) fonts)))
              (setf (game-state-font-path state) new)
              (save-options state)))
          (draw-label-fit ren font
                          (if (> n 0)
                              (format nil "~A (~D/~D)" label (1+ idx) n)
                              label)
                          (+ bx (+ (* 2 (+ btn-w btn-gap)) 6))
                          (+ yy 6)
                          :text
                          (- box-w (+ label-w (* 2 (+ btn-w btn-gap)) 64))))

        ;; Back
        (let* ((btn-w 140)
               (btn-x (+ box-x (- box-w btn-w 24)))
               (btn-y (+ box-y (- box-h 54))))
          (when (draw-button ren font "Back" btn-x btn-y btn-w 34
                             :tooltip "Return to the main menu.")
            (save-options state)
            (setf (game-state-screen state) :menu))))))
  state)
