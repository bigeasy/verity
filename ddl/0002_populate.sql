INSERT INTO Account (email, ready)
VALUES ('alan@prettyrobots.com', TRUE)
\g
INSERT INTO Plan (account_id, name)
VALUES (1, 'Example')
\g
INSERT INTO Series (plan_id, target, source)
VALUES (1, 'http://jekyellrb.com/', 'http://dossier:8048/verity/user/landing/index.js')
\g
