; AisleRiot - forty_thieves.scm
; Copyright (C) 2008 Ed Sirett  <ed@makewrite.demon.co.uk>
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
  (make-standard-double-deck)
  (shuffle-deck)

  (add-normal-slot DECK)

  (add-blank-slot)
; the foundations
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())
  (add-normal-slot '())

  (add-carriage-return-slot)
; the waste pile
  (add-extended-slot '() right)
  (add-carriage-return-slot)

; the tableau
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)
  (add-extended-slot '() down)

; these are the forty theives in the tableau
  (deal-cards-face-up 0 '(10 11 12 13 14 15 16 17 18 19))
  (deal-cards-face-up 0 '(10 11 12 13 14 15 16 17 18 19))
  (deal-cards-face-up 0 '(10 11 12 13 14 15 16 17 18 19))
  (deal-cards-face-up 0 '(10 11 12 13 14 15 16 17 18 19))

  (give-status-message)
; this is the return list of (new-game) and sets the size of the 
; the playing field.
  (list 10 4.5)
)

(define (in-tableau? slot) 
  (and (>= slot 10) (<= slot 19))
)

(define (in-foundation? slot) 
  (and (>= slot 1) (<= slot 8))
)

(define (in-tableau-or-waste? slot) 
  (or (in-tableau? slot) (= slot waste-pile))
)

(define waste-pile 9)
(define stock-pile 0) 
(define start-with-waste 9)
(define start-with-tableau 10)

(define (<> a b) 
   (not (= a b))
)

(define (give-status-message)
  (set-statusbar-message (get-stock-no-string)))

(define (get-stock-no-string)
  (string-append (_"Stock left:") " " 
		 (number->string (length (get-cards 0)))
  )
)

; Apparently this is used to allow a group of cards to be dragged. 
; if it returns #t then the cards are picked.
; single cards can always be pulled from waste or tableau
; multiple cards must be straight suit descending
; (droppable?) will sort out more restrictions later
 (define (button-pressed slot-id card-list)
  (and (not (empty-slot? slot-id))
       (in-tableau-or-waste? slot-id)
       ( or (= (length card-list) 1)
       	    (and (in-tableau? slot-id)
       	         (check-straight-descending-list card-list)
                 (check-same-suit-list card-list)	
	    )
       )
  )
)

;scoring  5*cards + 13 per suit completed.
(define (foundation-score slot-id prev-total)
  (define (current-total)
    (+ prev-total
       (* (length (get-cards slot-id)) 5)
       (if (= (length (get-cards slot-id)) 13)
           60
           0)))
  (if (= slot-id 8)
      (current-total)
      (foundation-score (+ slot-id 1) (current-total))))
       
(define (recalculate-score)
  (set-score!  (foundation-score 1 0)))

; counts empty slots in tableau
(define (space-score slot-id prev)
 (define (curtot previous) (+ previous (if (empty-slot? slot-id) 1 0)))
 (if (= slot-id 19) (curtot prev) (space-score (+ slot-id 1) (curtot prev)))
)
(define (tableau-spaces) 
   (space-score start-with-tableau 0)
)

; To save effort a pile of correctly descending same suit cards can be moved
; from the tableau to a foundation in one go.

( define (foundation-droppable? card-list f-slot) 
   (and (check-same-suit-list card-list)
        (check-straight-descending-list card-list) 
	(cond ( (empty-slot? f-slot)  
                    (= (get-value (car card-list)) ace) 
              )
              (	( = (get-value (car card-list)) (+ (get-value (get-top-card f-slot)) 1))
		    ( = (get-suit (get-top-card f-slot)) (get-suit (car card-list)))
	      )	
              (else #f)
	)
   )
)

; the maximum number of cards you can move as a short cut in one go 
; depends on the number of free tableau slot (it's 2^tableau slots)
; if the pile is going to an empty-slot than that slot is not really 
; an empty slot. If the pile is the entire contents of a tableau slot
; then (tableau-spaces) reports a 'false' extra space. hence the 
; extra code.
( define (max-move-in-tableau from-slot to-slot)
    (expt 2 (max 0 
                 (- 
		    (- (tableau-spaces) (if (empty-slot? to-slot) 1 0))
		       (if (empty-slot? from-slot) 1 0)
		 )
            )		
    )
)

; A bunch of cards may be dropped on to a tableau slot iff
; They are a descending same suit sequence that fits the top
; card of the tableau slot or an empty slot.
; this is a short cut to save moving cards individually
( define (tableau-droppable? s-slot card-list t-slot) 
   (and 
	(check-same-suit-list card-list)
        (check-straight-descending-list card-list)
	(<= (length card-list) (max-move-in-tableau s-slot t-slot))
	(cond ( (empty-slot? t-slot)  #t )
              (	( = (+ (get-value (car card-list)) (length card-list)) (get-value (get-top-card t-slot)) )
		    ( = (get-suit (get-top-card t-slot)) (get-suit (car card-list)))
	      )	
              (else #f) 
	)
   )
)


; droppable means that a list of cards coming from start-slot 
; and going to end-slot are valid to be moved. 
; picking up and dropping cards where they are is a null move.
; picking things off a foundation is a not permitted.
; dropping a valid pile onto a foundation is OK.
; if we are dropping onto another tableau pile sometimes OK.
; dropping card(s) elsewhere is not permitted.

(define (droppable?  start-slot card-list  end-slot) 
  (cond ( (= end-slot start-slot)  #f)
	( (in-foundation? start-slot) #f)
        ( (in-foundation? end-slot) (foundation-droppable? card-list end-slot) )
	( (in-tableau? end-slot) (tableau-droppable? start-slot card-list end-slot) )
	(else #f)
  )
)

;drop the dragged card(s) a pile of cards have to be revered 
; onto a foundation
(define (button-released start-slot card-list end-slot)
  (and (droppable? start-slot card-list end-slot)
       (if (in-tableau? end-slot) 
             (move-n-cards! start-slot end-slot card-list)
             (move-n-cards! start-slot end-slot (reverse card-list) )
       )
   )
)

; return "a move" if a card can be moved from from-slot to a foundation
; a move is a list either (#f) or (#t from-slot to-slot)
; no cards are actually moved this is a helper for both double-click
; and get-hint features.

(define (try-all-foundations from-slot card )
    (if (not (empty-slot? from-slot))
      (if (foundation-droppable? (list card) 1) 
        (list #t from-slot 1)
        (if (foundation-droppable? (list card) 2) 
	  (list #t from-slot 2)
          (if (foundation-droppable? (list card) 3) 
	    (list #t from-slot 3)
            (if (foundation-droppable? (list card) 4) 
	      (list #t from-slot 4)
              (if (foundation-droppable? (list card) 5) 
	         (list #t from-slot 5)
                 (if (foundation-droppable? (list card) 6) 
	           (list #t from-slot 6)
		   (if (foundation-droppable? (list card) 7) 
	             (list #t from-slot 7)
                     (if (foundation-droppable? (list card) 8) 
	               (list #t from-slot 8)
                       (list #f)
       ) ) ) ) ) ) ) )
       (list #f)
     )
)


; return a move if a card can be moved from from-slot to a tableau
; slot. This is a helper for hint, and double-click
(define (find-tableau-place from-slot card )
    (if (not (empty-slot? from-slot))
      (if (and (tableau-droppable? from-slot (list card) 10) (<> from-slot 10) )
        (list #t from-slot 10)
        (if (and (tableau-droppable? from-slot (list card) 11) (<> from-slot 11) )
	  (list #t from-slot 11)
          (if (and (tableau-droppable? from-slot (list card) 12) (<> from-slot 12) )
	    (list #t from-slot 12)
            (if (and (tableau-droppable? from-slot (list card) 13) (<> from-slot 13) )
	      (list #t from-slot 13)
              (if (and (tableau-droppable? from-slot (list card) 14) (<> from-slot 14) )
	        (list #t from-slot 14)
                (if (and (tableau-droppable? from-slot (list card) 15) (<> from-slot 15) )
	          (list #t from-slot 15)
                  (if (and (tableau-droppable? from-slot (list card) 16) (<> from-slot 16) )
	            (list #t from-slot 16)
                    (if (and (tableau-droppable? from-slot (list card) 17) (<> from-slot 17) )
	              (list #t from-slot 17)
                      (if (and (tableau-droppable? from-slot (list card) 18) (<> from-slot 18) )
	                (list #t from-slot 18)
                        (if (and (tableau-droppable? from-slot (list card) 19) (<> from-slot 19) )
	                  (list #t from-slot 19)
                          (list #f)
      ) ) ) ) ) ) ) ) ) ) 
      (list #f)
    )
)



(define (dealable?)
   (not (empty-slot? stock-pile)))

;deals cards from deck to waste
(define (button-clicked slot-id)
  (and (= slot-id stock-pile)
       (dealable?)
       (deal-cards-face-up stock-pile (list waste-pile))
  )
)

(define (do-deal-next-cards)
  (button-clicked stock-pile))

; if we can find a move to the foundations do it and return #t or #f.
(define (move-to-foundation) 
       (let ((move (find-any-move-to-foundation waste-pile))) 
	  (if (car move) (deal-cards-face-up (car (cdr move)) (list (car (reverse move))) ) #f ) 
       )
)

; search for any valid move to a foundation 
; helper code for both hint, autoplay
(define (find-any-move-to-foundation begin-slot) 
  (if (in-tableau-or-waste? begin-slot)
        (let ((test (try-all-foundations begin-slot (get-top-card begin-slot)) ))
             (if (car test) 
                 test 
                 (find-any-move-to-foundation (+ begin-slot 1)) 
             )
        )
        (list #f) 	
  )
)

; search for any valid move around the tableau 
; helper code for hint
(define (find-any-move-in-tableau begin-slot) 
  (if (in-tableau-or-waste? begin-slot)
        (let ((test (find-tableau-place begin-slot (get-top-card begin-slot)) ))
             (if (car test) 
                 test 
                 (find-any-move-in-tableau (+ begin-slot 1)) 
             )
        )
        (list #f) 	
  )
)



(define (autoplay-foundations)
(if (move-to-foundation) (delayed-call autoplay-foundations) #f)
)

; double click foundation for autoplay, otherwise does auto
; single move to foundation, or waste to tableau if poss.
(define (button-double-clicked slot-id)
  (cond ( (in-foundation? slot-id ) (autoplay-foundations))
        ( (in-tableau-or-waste? slot-id) 
            (let ((test (try-all-foundations slot-id (get-top-card slot-id)) ))
	      (if (car test) 
                 (deal-cards-face-up (car (cdr test)) (list (car (reverse test))) ) 
                 (let ((jump (find-tableau-place slot-id (get-top-card slot-id)) ))
                    (if (car jump) 
                       (deal-cards-face-up (car (cdr jump)) (list (car (reverse jump))) )
                       #f
                    )
                 )
              ) 
            )
          )
	(else #f)
   )
)


(define (game-continuable)
  (give-status-message)
  (and (not (game-won))
       (get-hint)
  )
)



(define (game-won)
  (= (recalculate-score) 1000)
)


;this is the last-straw hint maker
(define (check-for-deal)
  (if (not (empty-slot? stock-pile)) 
        (list 0 (_"Deal a card from stock"))
	 #f
  )
)

; turn a 'move' into a text description for get-hint.
(define (make-destination-hint slot)
    (if (in-foundation? slot)
       (if (empty-slot? slot) 
                (_"an empty foundation") 
		(get-name (get-top-card slot))
       )
       (if (empty-slot? slot) 
		(_"an empty space")
		(get-name (get-top-card slot)) 
       )
    )
) 

(define (make-hint move)
    (if (car move) 
       (list 2 (get-name (get-top-card (car (cdr move))))
               (make-destination-hint (car (reverse move))) 
       )     
       (list 0 (_"Bug! make-hint called on false move.") )
    )
)



; hint  suggests the following in order:
;  a move to a foundation from waste or tableau
;  move the top waste card to a valid tableau space or pile
;  move some other tableau card to another tableau space or pile 
;  deal a card or at end backup and try alternatives.
; these are not intended to be a the best moves simply to show 
; possible moves to help learn the rules.
(define (get-hint)
  (cond ( (car (find-any-move-to-foundation start-with-waste))
          (make-hint (find-any-move-to-foundation start-with-waste)) 
        ) 
        ( (and (not (empty-slot? waste-pile)) 
               (car (find-tableau-place waste-pile (get-top-card waste-pile) ) ) 
          )
          (make-hint (find-tableau-place waste-pile (get-top-card waste-pile)))
        )
        ( (car (find-any-move-in-tableau start-with-tableau) ) 
          (make-hint (find-any-move-in-tableau start-with-tableau ) )
        )
        (else (check-for-deal))
  )
)

(define (get-options) 
  #f)

(define (apply-options options) 
  #f)

(define (timeout) 
  #f)

(set-features droppable-feature dealable-feature)

(set-lambda new-game button-pressed button-released button-clicked
button-double-clicked game-continuable game-won get-hint get-options
apply-options timeout droppable? dealable?)
