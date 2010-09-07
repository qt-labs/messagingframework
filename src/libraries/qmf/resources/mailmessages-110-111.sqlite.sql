UPDATE mailmessages
SET mailfile = "qmfstoragemanager" || substr(mailfile, length("qtopiamailfile")+1, 1000000)
WHERE mailfile like "qtopiamailfile:%";
