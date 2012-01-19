CREATE TABLE mailthreads (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    messagecount INTEGER NOT NULL,
    unreadcount INTEGER NOT NULL,
    serveruid VARCHAR NOT NULL,
    parentaccountid INTEGER NOT NULL,
    subject VARCHAR,
    preview VARCHAR,
    senders VARCHAR,
    lastDate TIMESTAMP,
    startedDate TIMESTAMP,
    status INTEGER,
    FOREIGN KEY (parentaccountid) REFERENCES mailaccounts(id)
);
