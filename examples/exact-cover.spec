# Auxiliary predicate to determine whether a variable is integer. is_int/1 is declared as an input
# predicate so that anthem doesn’t generate its completed definition
input: is_int/1.
axiom: forall X (is_int(X) <-> exists N X = N).

# Perform the proofs under the assumption that n is a nonnegative integer input constant. n stands
# for the total number of input sets
input: n -> integer.
assume: n >= 0.

# s/2 is the input predicate defining the sets for which the program searches for exact covers
input: s/2.

# Perform the proofs under the assumption that the second parameter of s/2 (the number of the set)
# is always an integer
assume: forall X, Y (s(X, Y) -> is_int(Y)).

# Only valid sets can be included in the solution
assert: forall X (in(X) -> X >= 1 and X <= n).
# If an element is contained in an input set, it must be covered by all solutions
assert: forall X (exists I s(X, I) -> exists I (in(I) and s(X, I))).
# Elements may not be covered by two input sets
assert: forall I, J (exists X (s(X, I) and s(X, J)) and in(I) and in(J) -> I = J).
