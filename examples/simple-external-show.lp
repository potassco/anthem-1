s(X) :- p(X).
s(X) :- q(X).
u(X) :- r(X), not s(X).

#show u/1.

#external p(1).
#external q(1).
#external r(1).
