input: n -> integer.
output: prime/1.

# TODO: not necessary if using the lemma below in both directions
assume: n >= 1.

spec: forall X (prime(X) -> exists N (X = N)).
spec: forall N (prime(N) <-> N > 1 and N <= n and not exists I, J (I > 1 and J > 1 and I * J = N)).




lemma(backward): forall N (composite(N) <-> N > 1 and N <= n and exists I, J (I > 1 and J > 1 and I * J = N)).
