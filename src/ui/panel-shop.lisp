(in-package #:ember-forge)

(defparameter *shop-trade-qty* 10.0d0)

(defun shop-resource-available-p (state res)
  (and (not (eq res :coins))
       (> (sell-value res) 0.0d0)
       (or (resource-unlocked-p state res)
           (<= (resource-tier res) 1))))

(defun %shop-resource-list (state)
  (let ((out '()))
    (dolist (kw (all-resource-keys))
      (when (shop-resource-available-p state kw)
        (push kw out)))
    (nreverse out)))

(defun render-panel-shop (ren state font font-lg x y w h)
  (draw-panel-background ren :shop x y w h)
  (draw-label ren font-lg "SHOP" (+ x 20) (+ y 10) :text)

  (draw-label ren font
              (format nil "Sell bonus: ×~,2F    Prices: ×~,2F    Buy mult: ×~,2F"
                      (game-state-sell-mult state)
                      (game-state-shop-discount state)
                      (shop-buy-mult state))
              (+ x 24) (+ y 52) :text-dim)

  ;; Trade amount selector
  (let* ((sx (+ x 24))
         (sy (+ y 76))
         (btn-w 74)
         (btn-h 26)
         (gap 8))
    (draw-label ren font "Trade:" sx (+ sy 6) :text-dim)
    (let ((bx (+ sx 64)))
      (dolist (q '(1.0d0 10.0d0 100.0d0))
        (let ((active (= *shop-trade-qty* q)))
          (when (draw-button ren font (format nil "~D" (truncate q))
                             bx sy btn-w btn-h
                             :color (if active :btn-active :btn-normal))
            (setf *shop-trade-qty* q)))
        (incf bx (+ btn-w gap)))))

  (let* ((market-x (+ x 24))
         (market-y (+ y 116))
         (market-w (floor (* w 0.58)))
         (market-h (- h 140))
         (perks-x (+ market-x market-w 18))
         (perks-y market-y)
         (perks-w (- w (- perks-x x) 24))
         (row-h 34)
         (res-list (%shop-resource-list state)))
    ;; Market
    (draw-label ren font "MARKET" market-x (- market-y 18) :text)
    (let* ((qty *shop-trade-qty*)
           (btn-w 70)
           (btn-h (- row-h 10))
           (buy-mult (shop-buy-mult state)))
      (loop for res in res-list
            for i from 0
            for row-y = (+ market-y (* i (+ row-h 6)))
            while (< (+ row-y row-h) (+ market-y market-h))
            do (let* ((row-x market-x)
                      (row-w market-w)
                      (amt (resource-amount state res))
                      (cap (resource-cap state res))
                      (sell (sell-value res))
                      (sell-coins (* sell qty (game-state-sell-mult state)))
                      (buy-coins (* sell qty buy-mult (game-state-shop-discount state)))
                      (can-sell (can-sell-p state res qty))
                      (can-buy (shop-can-buy-resource-p state res qty))
                      (btn-buy-x (+ row-x (- row-w btn-w 8)))
                      (btn-sell-x (+ row-x (- row-w (* 2 (+ btn-w 8)))))
                      (text-right (- btn-sell-x 10)))
                 (draw-rect ren row-x row-y row-w row-h :bg-sidebar :border-color :btn-hover)
                 (draw-resource-icon ren res (+ row-x 10) (+ row-y 11))
                 (draw-label ren font (resource-name res) (+ row-x 30) (+ row-y 6) :text)
                 (draw-label ren font (format nil "~A / ~A" (fmt-number amt) (fmt-number cap))
                             (+ row-x 220) (+ row-y 6) :text-dim)
                 (draw-label ren font (format nil "Sell: +~A c" (fmt-number sell-coins))
                             text-right (+ row-y 6) :text-dim :align :right)
                 (draw-label ren font (format nil "Buy: -~A c" (fmt-number buy-coins))
                             text-right (+ row-y 20) :text-dim :align :right)
                 (when (draw-button ren font "Sell" btn-sell-x (+ row-y 5) btn-w btn-h
                                    :enabled can-sell
                                    :tooltip (format nil "Sell ~A ×~D."
                                                     (resource-name res)
                                                     (truncate qty)))
                   (sell-resource! state res qty))
                 (when (draw-button ren font "Buy" btn-buy-x (+ row-y 5) btn-w btn-h
                                    :enabled can-buy
                                    :tooltip (format nil "Buy ~A ×~D."
                                                     (resource-name res)
                                                     (truncate qty)))
                   (buy-resource! state res qty)))))

    ;; Perks
    (draw-label ren font "SHOP PERKS" perks-x (- perks-y 18) :text)
    (let* ((row-h 54)
           (i 0))
      (dolist (def *shop-items*)
        (when (or (shop-item-owned-p state (shop-item-def-id def))
                  (shop-item-unlocked-p state def))
          (let* ((id (shop-item-def-id def))
                 (lvl (shop-item-level state id))
                 (max (shop-item-def-max-level def))
                 (maxed (shop-item-maxed-p state def))
                 (cost (if maxed 0.0d0 (shop-item-next-cost state def)))
                 (can-buy (and (not maxed) (resource-has-p state :coins cost)))
                 (row-y (+ perks-y (* i (+ row-h 10))))
                 (btn-w 78)
                 (btn-h (- row-h 20))
                 (btn-x (+ perks-x (- perks-w btn-w 8)))
                 (btn-y (+ row-y 10))
                 (text-x (+ perks-x 10))
                 (text-right (- btn-x 10))
                 (title (if (> max 1)
                            (format nil "~A  (Lv ~D/~D)" (shop-item-def-name def) lvl max)
                            (if (= max 1)
                                (format nil "~A  (~:[Available~;Owned~])" (shop-item-def-name def) (> lvl 0))
                                (format nil "~A  (Lv ~D)" (shop-item-def-name def) lvl)))))
            (incf i)
            (draw-rect ren perks-x row-y perks-w row-h :bg-sidebar :border-color :btn-hover)
            (draw-label-fit ren font title text-x (+ row-y 6) :text (max 0 (- text-right text-x)))
            (draw-label-fit ren font (shop-item-def-description def) text-x (+ row-y 28) :text-dim
                            (max 0 (- text-right text-x)))
            (unless maxed
              (draw-label ren font (format nil "Cost: ~A c" (fmt-number cost))
                          text-right (+ row-y 6) :text-dim :align :right))
            (when (draw-button ren font (cond
                                          (maxed "Max")
                                          ((> lvl 0) "Buy")
                                          (t "Buy"))
                               btn-x btn-y btn-w btn-h
                               :enabled can-buy
                               :tooltip (if maxed "Fully upgraded."
                                            (format nil "Buy: ~A" (shop-item-def-name def))))
              (buy-shop-item! state id))))))
  state))
