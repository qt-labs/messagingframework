CREATE TABLE missingmessages (
    id INTEGER,
    identifier VARCHAR,
    PRIMARY KEY (id, identifier),
    FOREIGN KEY (id) REFERENCES mailmessages(id));
