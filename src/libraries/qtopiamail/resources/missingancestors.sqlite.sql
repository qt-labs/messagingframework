CREATE TABLE missingancestors (
    messageid INTEGER PRIMARY KEY,
    subjectid INTEGER,
    FOREIGN KEY (messageid) REFERENCES mailmessages(id),
    FOREIGN KEY (subjectid) REFERENCES mailsubjects(id));
