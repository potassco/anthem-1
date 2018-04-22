#show p/2.
%#external integer(n(0)).

{p(1..n, 1..n)}.

:- p(X, Y1), p(X, Y2), Y1 != Y2.
:- p(X1, Y), p(X2, Y), X1 != X2.

q1(X) :- p(X, _).
q2(Y) :- p(_, Y).

:- not q1(X), X = 1..n.
:- not q2(Y), Y = 1..n.
