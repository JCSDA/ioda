#define ODB_MAINCODE 1

#include "OOPS.h"

PUBLIC void 
OOPS_print_flags_file(void)
{
  int rc = 0, io = -1;
  FILE *fp = NULL;
  cma_open_(&io, "OOPS.flags", "w", &rc, strlen("OOPS.flags"), strlen("w"));
  if (rc != 1) return; /* multi-bin file ==> forget flags-file */
  fp = CMA_get_fp(&io);
  if (!fp) return; /* pointer NULL ==> forget the flags-file ;-( */
  cma_close_(&io, &rc);
}

PRIVATE int 
Create_Funcs(ODB_Pool *pool, int is_new, int io_method, int it)
{
  int nfuncs = 0;
  static int first_time = 1;
  static int IsConsidered[3];
  DRHOOK_START(Create_Funcs);
  if (first_time) {
    ODBMAC_INIT_IsConsidered(desc, 0);
    ODBMAC_INIT_IsConsidered(hdr, 1);
    ODBMAC_INIT_IsConsidered(body, 2);
    first_time = 0;
  } /* if (first_time) */
  ODBMAC_CREATE_TABLE(OOPS, desc, 0, 1);
  ODBMAC_CREATE_TABLE(OOPS, hdr, 1, 1);
  ODBMAC_CREATE_TABLE(OOPS, body, 2, 1);
  DRHOOK_END(0);
  return nfuncs;
}

PRIVATE int
Load_Pool(ODB_Pool *P, int io_method)
{
  int rc = 0;
  int Nbytes = 0;
  ODB_Funcs *pf = P->funcs;
  if (io_method != 5) {
    Call_TABLE_Load(desc, pf, 1);
    Call_TABLE_Load(hdr, pf, 1);
    Call_TABLE_Load(body, pf, 1);
  } /* if (io_method != 5) */
  return (rc < 0) ? rc : Nbytes;
}

PRIVATE int
Store_Pool(const ODB_Pool *P, int io_method)
{
  /* Data layout was compiled under Read/Only mode (-r option or ODB_READONLY=1) */
  return 0;
}

PUBLIC ODB_Funcs *
Anchor2OOPS(void *V, ODB_Pool *pool, int *ntables, int it, int add_vars)
{
  ODB_Anchor_Funcs *func = V;
  ODB_Pool *p = pool;
  DRHOOK_START(Anchor2OOPS);
  /* A special case : ntables not a NULL => return no. of tables */
  if (ntables) {
    *ntables = 3;
    goto finish;
  }
  func->create_funcs = Create_Funcs;
  func->load         = Load_Pool;
  func->store        = Store_Pool;
  if (add_vars) {
    p->add_var(p->dbname, "$mdi", NULL, it, USD_mdi_OOPS);
  } /* if (add_vars) */
  finish:
  DRHOOK_END(0);
  return NULL;
}
