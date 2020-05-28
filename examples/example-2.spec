# Perform the proofs under the assumption that n is a nonnegative integer input constant
input: n -> integer.
assume: n >= 0.

# p/1 is an auxiliary predicate
output: q/1.

# Verify that q computes the floor of the square root of n
spec: exists N (forall X (q(X) <-> X = N) and N >= 0 and N * N <= n and (N + 1) * (N + 1) > n).




# Multiplication with positive numbers preserves the order of integers
axiom: forall N1, N2, N3 (N1 > N2 and N3 > 0 -> N1 * N3 > N2 * N3).

# Induction principle instantiated for p.
# This axiom is necessary because we use Vampire without higher-order reasoning
axiom: forall N1 (p(N1) and forall N2 (N2 >= N1 and not p(N2) -> not p(N2 + 1)) -> forall N2 (N2 >= N1 -> p(N2))).
#axiom: p(0) and forall N (N >= 0 and p(N) -> p(N + 1)) -> forall N p(N).

#lemma(forward): forall N N * N >= N.
#lemma(forward): forall X (q(X) -> exists N X = N).
#lemma(forward): forall X (q(X) <-> exists N (X = N and N >= 0 and N * N <= n and not p(N + 1))).
#lemma(forward): exists N (q(N) <-> N >= 0 and N * N <= n and (N + 1) * (N + 1) > n).
#lemma(forward): exists N p(N).
#lemma(forward): forall N1, N2 (N2 > N1 and N1 >= 0 and p(N2) -> p(N1)).
lemma(forward): forall X (p(X) <-> exists N (X = N and N >= 0 and N * N <= n)).
lemma(forward): forall X (q(X) <-> exists N2 (X = N2 and N2 >= 0 and N2 * N2 <= n and (N2 + 1) * (N2 + 1) > n)).
lemma(forward): forall N1, N2 (N1 >= 0 and N2 >= 0 and N1 < N2 -> N1 * N1 < N2 * N2).
lemma(forward): forall N (N >= 0 and p(N + 1) -> p(N)).
lemma(forward): not p(n + 1).
lemma(forward): forall N1, N2 (q(N1) and N2 > N1 -> not q(N2)).
lemma(forward): forall N (N >= 0 and not p(N + 1) -> (N + 1) * (N + 1) > n).

lemma(backward): forall N1, N2 (q(N1) and q(N2) -> N1 = N2).
axiom: forall N1, N2 (p(N1) and not p(N1 + 1) and p(N2) and not p(N2 + 1) -> N1 = N2).
#lemma(backward): exists N (p(N) and not p(N + 1)).
#axiom: forall N (not p(N) -> not p(N + 1)).

lemma(backward): forall X1 (q(X1) -> p(X1) and exists X2 (exists N (X2 = N + 1 and N = X1) and not p(X2))).
#lemma(backward): forall X1 (q(X1) <- p(X1) and exists X2 (exists N (X2 = N + 1 and N = X1) and not p(X2))).
