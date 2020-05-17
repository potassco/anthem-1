{in(1..n)}.

:- I != J, in(I), in(J), s(X, I), s(X, J).
covered(X) :- in(I), s(X, I).
:- s(X, I), not covered(X).
