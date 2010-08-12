ALTER TABLE mailmessages ADD COLUMN copyserveruid VARCHAR NOT NULL DEFAULT '';
ALTER TABLE mailmessages ADD COLUMN restorefolderid INTEGER NOT NULL DEFAULT 0;
ALTER TABLE mailmessages ADD COLUMN listid VARCHAR NOT NULL DEFAULT '';
ALTER TABLE mailmessages ADD COLUMN rfcid VARCHAR NOT NULL DEFAULT '';

CREATE INDEX rfcid_idx ON mailmessages("rfcid");
