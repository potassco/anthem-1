assert:
	forall N
	(
		forall X (p(X) -> exists I exists M (I = M and I = X and I <= N))
		-> forall X (q(X) -> exists I exists M (I = M and I = X and I <= 2 * N))
	).
