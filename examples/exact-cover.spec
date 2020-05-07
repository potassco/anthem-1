axiom: forall X (is_int(X) <-> exists N X = N).

input: n -> integer, s/2, is_int/1.

assume: n >= 0.
assume: forall X, Y (s(X, Y) -> is_int(Y)).

assert: forall X (in(X) -> X >= 1 and X <= n).
assert: forall X (exists I s(X, I) -> exists I (in(I) and s(X, I))).
assert: forall I, J (exists X (s(X, I) and s(X, J)) and in(I) and in(J) -> I = J).
