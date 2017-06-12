colored(V, red) :- vertex(V), not colored(V, green), not colored(V, blue).
colored(V, green) :- vertex(V), not colored(V, red), not colored(V, blue).
colored(V, blue) :- vertex(V), not colored(V, red), not colored(V, green).

:- edge(V1, V2), colored(V1, C), colored(V2, C).

vertex(a).
vertex(b).
vertex(c).

edge(a, b).
edge(a, c).
