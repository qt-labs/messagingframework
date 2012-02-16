DELETE FROM mailthreads;
ALTER TABLE mailthreads ADD COLUMN subject VARCHAR;
ALTER TABLE mailthreads ADD COLUMN preview VARCHAR;
ALTER TABLE mailthreads ADD COLUMN senders VARCHAR;
ALTER TABLE mailthreads ADD COLUMN lastDate TIMESTAMP;
ALTER TABLE mailthreads ADD COLUMN startedDate TIMESTAMP;
ALTER TABLE mailthreads ADD COLUMN status INTEGER;
