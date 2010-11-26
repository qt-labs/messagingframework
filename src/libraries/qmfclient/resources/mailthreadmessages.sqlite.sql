CREATE TABLE mailthreadmessages (
    threadid INTEGER,
    messageid INTEGER,
    FOREIGN KEY (threadid) REFERENCES mailthreads(id),
    FOREIGN KEY (messageid) REFERENCES mailmessages(id));
    
CREATE INDEX threadmessageid_idx ON mailthreadmessages("messageid");
