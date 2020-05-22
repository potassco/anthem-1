input: n -> integer.
output: prime/1.

assume: n >= 1.

axiom: forall N1, N2, N3 (N1 > N2 and N3 > 0 -> N1 * N3 > N2 * N3).

lemma: forall N N + 0 = N.
lemma: forall I, J, N (I * J = N and I > 0 and N > 0 -> J > 0).

lemma(backward): forall X1 (composite(X1) <- (exists N1, N10 (X1 = N1 and 1 <= N1 and N1 <= n and 2 <= N10 and N10 <= N1 - 1 and exists N11 (N1 = (N10 * N11) and 0 < N11)))).
lemma(backward): forall N (composite(N) -> (exists N10 (1 <= N and N <= n and 2 <= N10 and N10 <= N - 1 and exists N11 (N = (N10 * N11) and 0 < N11)))).

lemma: forall X1 (composite(X1) <-> (exists N1, N10 (X1 = N1 and 1 <= N1 and N1 <= n and 2 <= N10 and N10 <= N1 - 1 and exists N11 (N1 = (N10 * N11) and 0 < N11)))).

#spec: forall X (composite(X) -> p__is_integer__(X)).
#spec: forall N (composite(N) <-> N > 1 and N <= n and exists I, J (I > 1 and J > 1 and I * J = N)).
spec: forall X (prime(X) -> p__is_integer__(X)).
spec: forall N (prime(N) <-> N > 1 and N <= n and not exists I, J (I > 1 and J > 1 and I * J = N)).
