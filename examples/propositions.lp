p :- q.
q :- not r.
{s; t} :- p.
:- s, not t.
:- r, q, p.

%#show p/0.
%#show q/0.
%#show r/0.
#show s/0.
#show t/0.
