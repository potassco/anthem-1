letter(a).
letter(b).
letter(c).

{p(1..3, Y)} :- letter(Y).
:- p(X1, Y), p(X2, Y), X1 != X2.

q(X) :- p(X, _).
:- X = 1..3, not q(X).

#show p/2.
