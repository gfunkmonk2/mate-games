; AisleRiot - treize.scm
; Copyright (C) 2001, 2003 Rosanna Yuen <zana@webwynk.net>
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

(define (new-game)
  (initialize-playing-area)  
  (set-ace-low)
  (make-standard-deck)
  (shuffle-deck)

  (add-normal-slot DECK)
  (add-normal-slot '())
  (set! HORIZPOS (- HORIZPOS (/ 2 3)))
  (add-extended-slot '() right)

  (add-carriage-return-slot)

  (add-blank-slot)
  (add-blank-slot)
  (add-blank-slot)
  (add-normal-slot '())

  (add-carriage-return-slot)
  (set! VERTPOS (- VERTPOS (/ 2 3)))
  (set! HORIZPOS (+ HORIZPOS 0.5))
  (add-blank-slot)
  (add-blank-slot)
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)
  (set! VERTPOS (- VERTPOS (/ 2 3)))
  (add-blank-slot)
  (add-blank-slot)
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)
  (set! VERTPOS (- VERTPOS (/ 2 3)))
  (set! HORIZPOS (+ HORIZPOS 0.5))
  (add-blank-slot)
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)
  (set! VERTPOS (- VERTPOS (/ 2 3)))
  (add-blank-slot)
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)
  (set! VERTPOS (- VERTPOS (/ 2 3)))
  (set! HORIZPOS (+ HORIZPOS 0.5))
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)
  (set! VERTPOS (- VERTPOS (/ 2 3)))
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (deal-cards-face-up 0 ' (3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
			     20 21 22 23 24 25 26 27 28 29 30))

  (give-status-message)

  (list 7 4))

(define (give-status-message)
  (set-statusbar-message (get-stock-no-string)))

(define (get-stock-no-string)
  (string-append (_"Stock left:") " " 
		 (number->string (length (get-cards 0)))))

(define (button-pressed slot-id card-list)
  (and (not (empty-slot? slot-id))
       (not (= (get-value (car card-list)) king))
       (available? slot-id 0)
       (= (length card-list) 1)))

(define (available? slot-id r-slot)
  (cond ((or (= slot-id 1)
	     (= slot-id 2))
	 #t)
	((= slot-id 0)
	 #f)
	((= slot-id 23)
	 (and (empty-slot? 29)
	      (empty-slot? 30)
	      (not (= r-slot 29))
	      (not (= r-slot 30))))
	((= slot-id 22)
	 (and (empty-slot? 28)
	      (empty-slot? 29)
	      (not (= r-slot 28))
	      (not (= r-slot 29))))
	((= slot-id 21)
	 (and (empty-slot? 27)
	      (empty-slot? 28)
	      (not (= r-slot 27))
	      (not (= r-slot 28))))
	((= slot-id 20)
	 (and (empty-slot? 26)
	      (empty-slot? 27)
	      (not (= r-slot 26))
	      (not (= r-slot 27))))
	((= slot-id 19)
	 (and (empty-slot? 25)
	      (empty-slot? 26)
	      (not (= r-slot 25))
	      (not (= r-slot 26))))
	((= slot-id 18)
	 (and (empty-slot? 24)
	      (empty-slot? 25)
	      (not (= r-slot 24))
	      (not (= r-slot 25))))
	((= slot-id 17)
	 (and (empty-slot? 22)
	      (empty-slot? 23)
	      (not (= r-slot 22))
	      (not (= r-slot 23))))
	((= slot-id 16)
	 (and (empty-slot? 21)
	      (empty-slot? 22)
	      (not (= r-slot 21))
	      (not (= r-slot 22))))
	((= slot-id 15)
	 (and (empty-slot? 20)
	      (empty-slot? 21)
	      (not (= r-slot 20))
	      (not (= r-slot 21))))
	((= slot-id 14)
	 (and (empty-slot? 19)
	      (empty-slot? 20)
	      (not (= r-slot 19))
	      (not (= r-slot 20))))
	((= slot-id 13)
	 (and (empty-slot? 18)
	      (empty-slot? 19)
	      (not (= r-slot 18))
	      (not (= r-slot 19))))
	((= slot-id 12)
	 (and (empty-slot? 16)
	      (empty-slot? 17)
	      (not (= r-slot 16))
	      (not (= r-slot 17))))
	((= slot-id 11)
	 (and (empty-slot? 15)
	      (empty-slot? 16)
	      (not (= r-slot 15))
	      (not (= r-slot 16))))
	((= slot-id 10)
	 (and (empty-slot? 14)
	      (empty-slot? 15)
	      (not (= r-slot 14))
	      (not (= r-slot 15))))
	((= slot-id 9)
	 (and (empty-slot? 13)
	      (empty-slot? 14)
	      (not (= r-slot 13))
	      (not (= r-slot 14))))
	((= slot-id 8)
	 (and (empty-slot? 11)
	      (empty-slot? 12)
	      (not (= r-slot 11))
	      (not (= r-slot 12))))
	((= slot-id 7)
	 (and (empty-slot? 10)
	      (empty-slot? 11)
	      (not (= r-slot 10))
	      (not (= r-slot 11))))
	((= slot-id 6)
	 (and (empty-slot? 9)
	      (empty-slot? 10)
	      (not (= r-slot 9))
	      (not (= r-slot 10))))
	((= slot-id 5)
	 (and (empty-slot? 7)
	      (empty-slot? 8)
	      (not (= r-slot 7))
	      (not (= r-slot 8))))
	((= slot-id 4)
	 (and (empty-slot? 6)
	      (empty-slot? 7)
	      (not (= r-slot 6))
	      (not (= r-slot 7))))
	((= slot-id 3)
	 (and (empty-slot? 4)
	      (empty-slot? 5)
	      (not (= r-slot 4))
	      (not (= r-slot 5))))))

(define (droppable? start-slot card-list end-slot)
  (and (not (empty-slot? end-slot))
       (available? end-slot start-slot)
       (= 13 (+ (get-value (car card-list))
		(get-value (get-top-card end-slot))))))

(define (button-released start-slot card-list end-slot)
  (and (droppable? start-slot card-list end-slot)
       (add-to-score! 2)
       (remove-card end-slot)
       (if (or (= start-slot 1)
	       (= end-slot 1))
	   (if (not (empty-slot? 2))
	       (begin
		 (let ((new-contents (get-cards 2)))
		   (let ((moving-back (car (reverse new-contents))))
		     (set-cards! 1 (list moving-back)))
		   (set-cards! 2 (reverse (cdr (reverse new-contents))))))
	       #t)
	   #t)))

(define (button-clicked slot-id)
  (if (= slot-id 0)
      (if (empty-slot? 1)
	  (and (not (empty-slot? 0))
	       (deal-cards-face-up 0 '(1)))
	  (and (not (empty-slot? 0))
	       (deal-cards-face-up 0 '(2))))
      (and (not (empty-slot? slot-id))
	   (available? slot-id 0)
	   (= (get-value (get-top-card slot-id))
	      king)
	   (remove-card slot-id)
	   (if (= slot-id 1)
	       (if (not (empty-slot? 2))
		   (begin
		     (let ((new-contents (get-cards 2)))
		       (let ((moving-back (car (reverse new-contents))))
			 (set-cards! 1 (list moving-back)))
		       (set-cards! 2 (reverse (cdr (reverse new-contents))))))))
	   (add-to-score! 1))))

(define (button-double-clicked slot-id)
  #f)

(define (game-continuable)
  (give-status-message)
  (and (not (game-won))
       (get-hint)))

(define (game-won)
  (and (empty-slot? 3)
       (empty-slot? 1)
       (empty-slot? 0)))

(define (check-move slot1 slot2)
  (if (or (empty-slot? slot1)
	  (not (available? slot1 0)))
      (if (< slot1 29)
	  (check-move (+ 1 slot1) (+ 2 slot1))
	  #f)
      (if (= king (get-value (get-top-card slot1)))
	  (list 2 (get-name (get-top-card slot1)) (_"itself"))
	  (if (or (empty-slot? slot2)
		  (not (available? slot2 0))
		  (not (= 13 (+ (get-value (get-top-card slot1))
				(get-value (get-top-card slot2))))))
	      (if (< slot2 30)
		  (check-move slot1 (+ 1 slot2))
		  (if (< slot1 29)
		      (check-move (+ 1 slot1) (+ 2 slot1))
		      #f))
	      (list 1 
		    (get-name (get-top-card slot1)) 
		    (get-name (get-top-card slot2)))))))

(define (dealable?)
  (if (not (empty-slot? 0))
      (list 0 (_"Deal a card"))
      #f))

(define (check-waste)
  (and (not (empty-slot? 2))
       (> (length (get-cards 2)) 1)
       (= 13 (+ (get-value (get-top-card 2))
		(get-value (cadr (get-cards 2)))))
       (list 1
	     (get-name (get-top-card 2))
	     (get-name (cadr (get-cards 2))))))

(define (get-hint)
  (or (check-move 1 2)  
      (check-waste)
      (dealable?)))

(define (get-options) 
  #f)

(define (apply-options options) 
  #f)

(define (timeout) 
  #f)

(set-features droppable-feature)

(set-lambda new-game button-pressed button-released button-clicked
button-double-clicked game-continuable game-won get-hint get-options
apply-options timeout droppable?)
