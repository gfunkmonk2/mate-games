; AisleRiot - spiderette.scm
; Copyright (C) 2001 Rosanna Yuen <zana@webwynk.net>
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

(load "spider.scm")

(define stock 0)
(define foundation '(1 2 3 4))
(define tableau '(5 6 7 8 9 10 11))
(define winning-score 48)

(define (new-game)
  (initialize-playing-area)
  (set-ace-low)
  (make-standard-deck)
  (shuffle-deck)

  (add-normal-slot DECK)

  (add-blank-slot)
  (add-blank-slot)

  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)

  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)

  (deal-cards 0 '(5 6 7 8 9 10 11 6 7 8 9 10 11 7 8 9 10 11 8 9 10 11
		    9 10 11 10 11 11))
  (map flip-top-card '(5 6 7 8 9 10 11))

  (give-status-message)

  (list 7 4))

(define (get-options) #f)

(define (apply-options options) #f)

(set-lambda new-game button-pressed button-released button-clicked button-double-clicked game-over game-won get-hint get-options apply-options timeout droppable?)
