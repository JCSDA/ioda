CREATE TABLE desc AS (
  andate YYYYMMDD,  
  antime HHMMSS,  
  hdr @LINK,  
);

CREATE TABLE hdr AS (
  seqno pk1int,
  date YYYYMMDD,
  time HHMMSS,
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

