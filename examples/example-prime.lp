% Prime numbers from 1 to n.
composite(N) :- N = 1..n, I = 2..(N - 1), N \ I = 0.
prime(N) :- N = 2..n, not composite(N).
