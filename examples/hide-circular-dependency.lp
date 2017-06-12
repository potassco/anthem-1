a(X) :- circle1(X).

circle1(X) :- circle2(X).
circle2(X) :- circle3(X).
circle3(X) :- circle1(X).

#show a/1.
%#show circle1/1.
%#show circle2/1.
%#show circle3/1.
