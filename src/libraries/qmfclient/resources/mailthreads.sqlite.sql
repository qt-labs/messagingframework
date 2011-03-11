CREATE TABLE mailthreads (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    messagecount INTEGER NOT NULL,
    unreadcount INTEGER NOT NULL,
    serveruid VARCHAR NOT NULL
);
