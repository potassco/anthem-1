#external color(1).
#external edge(2).
#external vertex(1).
#show color/2.

{color(V, C)} :- vertex(V), color(C).
covered(V) :- color(V, _).
:- vertex(V), not covered(V).
:- color(V1, C), color(V2, C), edge(V1, V2).
:- color(V, C1), color(V, C2), C1 != C2.
