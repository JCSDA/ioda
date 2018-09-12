
// Schema for database 'OOPS'
// File created on 20180621 152722

SET $mdi = 2147483647;

CREATE TABLE desc AS (
  andate yyyymmdd,
  antime hhmmss,
  hdr @LINK,
);

CREATE TABLE hdr AS (
  seqno pk1int,
  date yyyymmdd,
  time hhmmss,
  lat pk9real,
  lon pk9real,
  body @LINK,
);

CREATE TABLE body AS (
  varno pk1int,
  obsvalue pk9real,
  entryno pk1int,
  vertco_reference_1 pk9real,
);
