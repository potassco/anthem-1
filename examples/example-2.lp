p(X) :- X = 0..n, X * X <= n.
q(X) :- p(X), not p(X + 1).
