% assign a set of colors to each vertex
{color(V, C)} :- vertex(V), color(C).

% at most one color per vertex
:- color(V, C1), color(V, C2), C1 != C2.

% at least one color per vertex
covered(V) :- color(V, _).
:- vertex(V), not covered(V).

% adjacent vertices don’t share the same color
:- color(V1, C), color(V2, C), edge(V1, V2).

#show color/2.

#external vertex(1).
#external edge(2).
#external color(1).
