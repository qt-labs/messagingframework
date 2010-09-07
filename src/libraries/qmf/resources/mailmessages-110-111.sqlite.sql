UPDATE mailmessages
SET mailfile = REPLACE(mailfile, 'qtopiamailfile:', 'qmfstoragemanager:');
