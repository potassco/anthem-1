# Perform the proofs under the assumption that n is a nonnegative integer input constant
input: n -> integer.
assume: n >= 0.

# p/1 is an auxiliary predicate
output: q/1.

# Verify that q computes the floor of the square root of n
spec: exists N (forall X (q(X) <-> X = N) and N >= 0 and N * N <= n and (N + 1) * (N + 1) > n).
