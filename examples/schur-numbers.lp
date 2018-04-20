#show in/2.
#external integer(n(0)).
#external integer(r(0)).

{in(1..n, 1..r)}.
covered(I) :- in(I, S).

:- I = 1..n, not covered(I).
:- in(I, S), in(J, S), in(I + J, S).
