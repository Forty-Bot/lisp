; lisp-forty, a lisp interpreter
; Copyright (C) 2014-16 Sean Anderson
;
; This file is part of lisp-forty.
;
; lisp-forty is free software: you can redistribute it and/or modify it under
; the terms of the GNU General Public License as published by the Free Software
; Foundation, either version 3 of the License, or (at your option) any later
; version.
;
; lisp-forty is distributed in the hope that it will be useful, but WITHOUT
; ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
; FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along with
; lisp-forty  If not, see <http://www.gnu.org/licenses/>.

(def {nil} {})

(def {fun} (\ {args body}
  {def (head args) (\ (tail args) body)
}))

(fun {unpack f xs} {
  eval (join (list f) xs)
})
(fun {pack f & xs} {f xs})

(def {curry} unpack)
(def {uncurry} pack)

(fun {do & l} {
  if (eq l nil)
    {nil}
    {last l}
})

(fun {let b} {
  ((\ {_} b) ())
})

(fun {not n} {nand n n})
(fun {or x y} {nand (not x) (not y)})
(fun {and x y} {not (nand x y)})

;misc
(fun {flip f a b} {f b a})
(fun {comp f g x} {f (g x)})

;List funcs
(fun {fst l} {eval (head l)})
(fun {snd l} {eval (head (tail l))})
(fun {trd l} {eval (head (tail (tail l)))})

(fun {len l} {
  if (eq l nil)
    {0}
    {+ 1 (len (tail l))}
})

;Nth item
(fun {nth n l} {
  if(eq n 0)
    {fst l}
    {nth (- n 1) (tail l)}
})

(fun {last l} {nth (- (len l) 1) l})

(fun {take n l} {
  if (eq n 0)
    {nil}
    {join (head l) (take (- n 1) (tail l))}
})

(fun {drop n l} {
  if (eq n 0)
    {l}
    {drop (- n 1) (tail l)}
})

(fun {split n l} {list (split n l) (drop n l)})

(fun {elem x l} {
  if (eq l nil)
    {false}
    {if (eq x (fst l))
      {true}
      {elem x (tail l)}}
})

(fun {map f l} {
  if (eq l nil)
    {nil}
    {join (list (f (fst l))) (map f (tail l))}
})

(fun {filter f l} {
  if (eq l nil)
    {nil}
    {join (if (f (fst l))
      {head l}
      {nil}
    ) (filter f (tail l))}
})

(fun {foldl f z l} {
  if (eq l nil)
    {z}
    {foldl f (f z (fst l)) (tail l)}
})

(fun {sum l} {foldl + 0 l})
(fun {product l} {foldl * 1 l})

(fun {select & cs} {
  if (eq cs nil)
    {error "No selection found!"}
    {if (fst (fst cs))
      {snd (fst cs)}
      {unpack select (tail cs)}}
})

(def {otherwise} true)

(fun {case x & cs} {
  if (eq cs nil)
    {error "No Case Found"}
    {if (eq x (fst (fst cs))) {snd (fst cs)} {unpack case (join (list x) (tail cs))}}
})

(fun {range begin end} {
  if (gt begin end)
    {nil}
    {join (list begin) (range (+ begin 1) end)}
})
