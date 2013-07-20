CREATE OR REPLACE FUNCTION update_timestamp() RETURNS TRIGGER AS $update_timestamp$
    BEGIN
        NEW.modified := current_timestamp;
        RETURN NEW;
    END;
$update_timestamp$ LANGUAGE plpgsql
\g
CREATE TABLE Account (
    id              SERIAL NOT NULL,
    email           VARCHAR(255) NOT NULL,
    ready           BOOLEAN NOT NULL DEFAULT FALSE,
    modified        TIMESTAMP DEFAULT now(),
    created         TIMESTAMP DEFAULT now(),
    PRIMARY KEY (id)
)
\g
CREATE TRIGGER update_timestamp BEFORE INSERT OR UPDATE ON Account
FOR EACH ROW EXECUTE PROCEDURE update_timestamp()
\g
CREATE TABLE Plan (
    id              SERIAL NOT NULL,
    account_id      INTEGER NOT NULL,
    name            VARCHAR(255) NOT NULL,
    modified        TIMESTAMP DEFAULT now(),
    created         TIMESTAMP DEFAULT now(),
    PRIMARY KEY (id)
)
\g
CREATE TRIGGER update_timestamp BEFORE INSERT OR UPDATE ON Plan
FOR EACH ROW EXECUTE PROCEDURE update_timestamp()
\g
CREATE TABLE Series (
    id              SERIAL NOT NULL,
    plan_id         INTEGER NOT NULL,
    target          VARCHAR(1024) NOT NULL,
    source          VARCHAR(1024) NOT NULL,
    modified        TIMESTAMP DEFAULT now(),
    created         TIMESTAMP DEFAULT now(),
    PRIMARY KEY (id)
)
\g
CREATE TRIGGER update_timestamp BEFORE INSERT OR UPDATE ON Series
FOR EACH ROW EXECUTE PROCEDURE update_timestamp()
\g
