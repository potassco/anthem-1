a(X, Y) :- b(c(X - 1), d(e(1 + Y))).
b(c(X), d(Y)) :- c(X), d(Y).
c(1..3).
d(e(8..9)).

#show a/2.
%#show b/2.
%#show c/1.
%#show d/1.
