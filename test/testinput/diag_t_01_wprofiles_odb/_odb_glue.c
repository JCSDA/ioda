#include <stdio.h>
#include <string.h>

extern void
codb_procdata_(int *myproc,
               int *nproc,
               int *pid,
               int *it,
               int *inumt);

extern void 
ODB_add2funclist(const char *dbname,
		 void (*func)(),
                 int funcno);

#define Static_Init(db) \
if (strncmp(dbname, #db, dbname_len) == 0) { \
  extern void db##_static_init(); \
  ODB_add2funclist(#db, db##_static_init, 0); \
} \
else { /* fprintf(stderr,"***Warning: Not initializing '%s'\n",#db); */ }  

void
codb_set_entrypoint_(const char *dbname
	             /* Hidden arguments */
	             , int dbname_len)
{
  int myproc = 0;
  codb_procdata_(&myproc, NULL, NULL, NULL, NULL);
  if (myproc == 1) {
    fprintf(stderr,
            "codb_set_entrypoint_(dbname='%*s', dbname_len=%d)\n",
            dbname_len, dbname, dbname_len);
  }
  Static_Init(OOPS);
}
