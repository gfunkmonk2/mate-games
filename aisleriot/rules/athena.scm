;; AisleRiot - athena.scm  -*-scheme-*- 
;; Copyright (C) Alan Horkan, 2005.  
;; based on klondike.scm
; Copyright (C) 1998, 2003 Jonathan Blandford <jrb@mit.edu>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;; Athena differs from Klondike only in the intial layout
;; including 1 or 3 card deal, and any other options like ...
;; Optional "King Only" enabled by default [TODO]
;; TODO rewrite in a way less redundant way and share code with Klondike
;; As seen in Pretty Good Solitaire 10  http://goodsol.com  2005.  

(load "klondike.scm")

(define deal-one #t)
(define deal-three #f)
(define no-redeal #f)

(define max-redeal 2)

; The set up:

(define tableau '(6 7 8 9 10 11 12))
(define foundation '(2 3 4 5))
(define stock 0)
(define waste 1)

(define (new-game)
  (initialize-playing-area)
  (set-ace-low)

  (make-standard-deck)
  (shuffle-deck)
  
  (add-normal-slot DECK 'stock)

  (if deal-three
      (add-partially-extended-slot '() right 3 'waste)
      (add-normal-slot '() 'waste))

  (add-blank-slot)
  (add-normal-slot '() 'foundation)
  (add-normal-slot '() 'foundation)
  (add-normal-slot '() 'foundation)
  (add-normal-slot '() 'foundation)
  (add-carriage-return-slot)
  (add-extended-slot '() down 'tableau)
  (add-extended-slot '() down 'tableau)
  (add-extended-slot '() down 'tableau)
  (add-extended-slot '() down 'tableau)
  (add-extended-slot '() down 'tableau)
  (add-extended-slot '() down 'tableau)
  (add-extended-slot '() down 'tableau)

  (deal-cards stock tableau) 
  (deal-cards-face-up stock tableau) 
  (deal-cards stock tableau) 
  (deal-cards-face-up stock tableau) 

  (give-status-message)

  (list 7 3)
)

(define (get-options)
  (list (list (_"Three card deals") deal-three)))

(define (apply-options options)
  (set! deal-three (cadar options)))

(set-lambda new-game button-pressed button-released button-clicked button-double-clicked game-over game-won get-hint get-options apply-options timeout droppable? dealable?)
