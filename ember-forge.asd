(asdf:defsystem #:ember-forge
  :author "Bruno"
  :description "Ember Forge — idle/incremental game in Common Lisp + SDL2"
  :version "1.0.0"
  :depends-on (#:alexandria #:sdl2 #:sdl2-ttf #:sdl2-image #:uiop)
  :serial t
  :components
  ((:module "src"
    :serial t
    :components
    ((:file "package")
     (:file "state")
     (:module "util"
      :serial t
      :components
      ((:file "math")
       (:file "timer")
       (:file "fonts")
       (:file "options")
       (:file "save")))
     (:file "resources")
     (:file "buildings")
     (:file "recipes")
     (:file "upgrades")
     (:file "events")
     (:file "prestige")
     (:file "shop")
     (:file "game")
     (:module "ui"
      :serial t
      :components
      ((:file "theme")
       (:file "widgets")
       (:file "backgrounds")
       (:file "notifs")
       (:file "tooltip")
       (:file "menu")
       (:file "panel-forge")
       (:file "panel-mine")
       (:file "panel-upgrades")
       (:file "panel-recipes")
       (:file "panel-shop")
       (:file "panel-prestige")
       (:file "renderer")))
     (:file "main")))))
