#show in/2.

{in(1..n, 1..r)}.
covered(I) :- in(I, S).

:- I = 1..n, not covered(I).
:- in(I, S), in(J, S), in(I + J, S).
