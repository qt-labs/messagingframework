CREATE TABLE mailmessageidentifiers (
    id INTEGER,
    identifier VARCHAR UNIQUE,
    FOREIGN KEY (id) REFERENCES mailmessages(id));
