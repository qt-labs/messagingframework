UPDATE mailmessages SET status = status - (SELECT 1 << (statusbit-1) FROM mailstatusflags WHERE name = "Read") WHERE status & (SELECT 1 << (statusbit-1) FROM mailstatusflags WHERE name = "Read");

UPDATE mailmessages SET status = status | (SELECT 1 << (statusbit-1) FROM mailstatusflags WHERE name = "Read") WHERE status & (SELECT 1 << (statusbit-1) FROM mailstatusflags WHERE name = "ReadElsewhere");
