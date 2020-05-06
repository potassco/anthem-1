axiom: forall X (isint(X) <-> exists N X = N).
axiom: forall N1, N2, N3 (N1 > N2 and N3 > 0 -> N1 * N3 > N2 * N3).

axiom: p(0) and forall N (N >= 0 and p(N) -> p(N + 1)).

input: n -> integer.

assumption: n >= 0.

assertion: exists N (forall X (q(X) <-> X = N) and N >= 0 and N * N <= n and (N + 1) * (N + 1) > n).



lemma(forward): forall N N * N >= N.
lemma(forward): forall X (q(X) -> exists N X = N).
lemma(forward): forall X (p(X) <-> exists N2 (X = N2 and N2 >= 0 and N2 * N2 <= n)).
lemma(forward): forall X (q(X) <-> exists N2 (X = N2 and N2 >= 0 and N2 * N2 <= n and not p(N2 + 1))).
lemma(forward): forall N2 (N2 >= 0 and not p(N2 + 1) -> (N2 + 1) * (N2 + 1) > n).
lemma(forward): forall X (q(X) <-> exists N2 (X = N2 and N2 >= 0 and N2 * N2 <= n and (N2 + 1) * (N2 + 1) > n)).
lemma(forward): exists N2 (forall X (X = N2 -> (q(X) <-> N2 >= 0 and N2 * N2 <= n and (N2 + 1) * (N2 + 1) > n))).
