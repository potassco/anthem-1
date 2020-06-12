% After eliminating the super-easy aggregate expression from the rule
%
% {color(X,Z) : color(Z)} = 1 :- vertex(X).
%
% we get 4 rules:

{color(X,Z)} :- vertex(X), color(Z).
:- color(X,Z1), color(X,Z2), vertex(X), color(Z1), color(Z2), Z1 != Z2.
aux(X) :- vertex(X), color(Z), color(X,Z).
:- vertex(X), not aux(X).

:- edge(X,Y), color(X,Z), color(Y,Z).
