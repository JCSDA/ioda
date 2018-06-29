#undef ODB_MAINCODE
#undef IS_a_VIEW
#define IS_a_TABLE_desc 1
#include "OOPS.h"

extern double USD_mdi_OOPS; /* $mdi */


PUBLIC int
OOPS_Pack_T_desc(void *T)
{
  int Nbytes = 0;
  TABLE_desc *P = T;
  Packed_DS *PDS;
  if (P->Is_loaded) {
    PDS = PackDS(P, OOPS, yyyymmdd, andate); CHECK_PDS_ERROR(1);
    PDS = PackDS(P, OOPS, hhmmss, antime); CHECK_PDS_ERROR(2);
    PDS = PackDS(P, OOPS, linkoffset_t, LINKOFFSET(hdr)); CHECK_PDS_ERROR(3);
    PDS = PackDS(P, OOPS, linklen_t, LINKLEN(hdr)); CHECK_PDS_ERROR(4);
  }
  return Nbytes;
}

PUBLIC int
OOPS_Unpack_T_desc(void *T)
{
  int Nbytes = 0;
  TABLE_desc *P = T;
  if (P->Is_loaded) {
    UseDS(P, OOPS, yyyymmdd, andate); Nbytes += BYTESIZE(P->andate.d);
    UseDS(P, OOPS, hhmmss, antime); Nbytes += BYTESIZE(P->antime.d);
    UseDS(P, OOPS, linkoffset_t, LINKOFFSET(hdr)); Nbytes += BYTESIZE(P->LINKOFFSET(hdr).d);
    UseDS(P, OOPS, linklen_t, LINKLEN(hdr)); Nbytes += BYTESIZE(P->LINKLEN(hdr).d);
  }
  return Nbytes;
}

PUBLIC int
OOPS_Sel_T_desc(void *T, ODB_PE_Info *PEinfo, int phase, void *feedback)
{
  TABLE_desc *P = T;
  ODBMAC_TABLE_DELAYED_LOAD(desc);
  return P->Nrows;
}


PreGetTable(OOPS, d, double, desc)
  Call_CopyGet_TABLE(OOPS, d, 1, desc, yyyymmdd, D, andate, Count, DATATYPE_YYYYMMDD);
  Call_CopyGet_TABLE(OOPS, d, 2, desc, hhmmss, D, antime, Count, DATATYPE_HHMMSS);
  Call_CopyGet_TABLE(OOPS, d, 3, desc, linkoffset_t, D, LINKOFFSET(hdr), Count, DATATYPE_LINKOFFSET);
  Call_CopyGet_TABLE(OOPS, d, 4, desc, linklen_t, D, LINKLEN(hdr), Count, DATATYPE_LINKLEN);
PostGetTable(d, double, desc)


PrePutTable(OOPS, d, double, desc)
  Call_CopyPut_TABLE(OOPS, d, 1, desc, yyyymmdd, andate, D, Count, DATATYPE_YYYYMMDD);
  Call_CopyPut_TABLE(OOPS, d, 2, desc, hhmmss, antime, D, Count, DATATYPE_HHMMSS);
  Call_CopyPut_TABLE(OOPS, d, 3, desc, linkoffset_t, LINKOFFSET(hdr), D, Count, DATATYPE_LINKOFFSET);
  Call_CopyPut_TABLE(OOPS, d, 4, desc, linklen_t, LINKLEN(hdr), D, Count, DATATYPE_LINKLEN);
PostPutTable(d, double, desc)

PreLoadTable(OOPS, desc);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, yyyymmdd, DATATYPE_YYYYMMDD, andate);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, hhmmss, DATATYPE_HHMMSS, antime);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, linkoffset_t, DATATYPE_LINKOFFSET, LINKOFFSET(hdr));
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, linklen_t, DATATYPE_LINKLEN, LINKLEN(hdr));
PostLoadTable(desc)

PreStoreTable(OOPS, desc);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, yyyymmdd, DATATYPE_YYYYMMDD, andate);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, hhmmss, DATATYPE_HHMMSS, antime);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, linkoffset_t, DATATYPE_LINKOFFSET, LINKOFFSET(hdr));
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, linklen_t, DATATYPE_LINKLEN, LINKLEN(hdr));
PostStoreTable(desc)

DefineLookupTable(desc)

PUBLIC void
OOPS_Dim_T_desc(void *T, int *Nrows, int *Ncols,
  int *Nrowoffset, int ProcID)
{
  TABLE_desc *P = T;
  Call_LookupTable(desc, P, Nrows, Ncols);
  if (Nrowoffset) *Nrowoffset = 0;
}

PUBLIC void
OOPS_Swapout_T_desc(void *T)
{
  TABLE_desc *P = T;
  int Nbytes = 0;
  int Count = 0;
  int PoolNo = P->PoolNo;
  FILE *do_trace = NULL;
  if (P->Swapped_out || !P->Is_loaded) return;
  do_trace = ODB_trace_fp();
  FreeDS(P, andate, Nbytes, Count);
  FreeDS(P, antime, Nbytes, Count);
  FreeDS(P, LINKOFFSET(hdr), Nbytes, Count);
  FreeDS(P, LINKLEN(hdr), Nbytes, Count);
  P->Nrows = 0;
  P->Nalloc = 0;
  P->Is_loaded = 0;
  P->Swapped_out = P->Is_new ? 0 : 1;
  ODBMAC_TRACE_SWAPOUT(desc,4);
}

DefineRemoveTable(OOPS, desc)

PUBLIC int
OOPS_Sql_T_desc(FILE *fp, int mode, const char *prefix, const char *postfix, char **sqlout) { ODBMAC_TABLESQL(); }

PUBLIC void *
OOPS_Init_T_desc(void *T, ODB_Pool *Pool, int Is_new, int IO_method, int it, int dummy)
{
  TABLE_desc *P = T;
  int PoolNo = Pool->poolno;
  ODB_Funcs *pf;
  static ODB_CommonFuncs *pfcom = NULL; /* Shared between pools & threads */
  DRHOOK_START(OOPS_Init_T_desc);
  if (!P) ALLOC(P, 1);
  PreInitTable(P, 4);
  InitDS(yyyymmdd, DATATYPE_YYYYMMDD, andate, desc, 1);
  InitDS(hhmmss, DATATYPE_HHMMSS, antime, desc, 1);
  InitDS(linkoffset_t, DATATYPE_LINKOFFSET, LINKOFFSET(hdr), desc, 1);
  InitDS(linklen_t, DATATYPE_LINKLEN, LINKLEN(hdr), desc, 1);
  if (!pfcom) { /* Initialize once only */
    CALLOC(pfcom,1);
    { static char s[] = "@desc"; pfcom->name = s; }
    pfcom->is_table = 1;
    pfcom->is_considered = 0;
    pfcom->ntables = 0;
    pfcom->ncols = 4;
    pfcom->tableno = 0;
    pfcom->rank = 0;
    pfcom->wt = 0.000000;
    pfcom->tags = OOPS_Set_T_desc_TAG(&pfcom->ntag, &pfcom->nmem);
    pfcom->preptags = OOPS_Set_T_desc_PREPTAG(&pfcom->npreptag);
    pfcom->Info = NULL;
    pfcom->create_index = 0;
    pfcom->init = OOPS_Init_T_desc;
    pfcom->swapout = OOPS_Swapout_T_desc;
    pfcom->dim = OOPS_Dim_T_desc;
    pfcom->sortkeys = NULL;
    pfcom->update_info = NULL;
    pfcom->aggr_info = NULL;
    pfcom->getindex = NULL; /* N/A */
    pfcom->putindex = NULL; /* N/A */
    pfcom->select = OOPS_Sel_T_desc;
    pfcom->remove = OOPS_Remove_T_desc;
    pfcom->peinfo = NULL; /* N/A */
    pfcom->cancel = NULL;
    pfcom->dget = OOPS_dGet_T_desc; /* REAL(8) dbmgr */
    pfcom->dput = OOPS_dPut_T_desc; /* REAL(8) dbmgr */
    pfcom->load = OOPS_Load_T_desc;
    pfcom->store = OOPS_Store_T_desc;
    pfcom->pack = OOPS_Pack_T_desc;
    pfcom->unpack = OOPS_Unpack_T_desc;
    pfcom->sql = OOPS_Sql_T_desc;
    pfcom->ncols_aux = 0;
    pfcom->colaux = NULL;
    pfcom->has_select_distinct = 0;
    pfcom->has_usddothash = 0;
  } /* if (!pfcom) */
  ALLOC(pf, 1);
  pf->it = it;
  pf->data = P;
  pf->Res = NULL;
  pf->tmp = NULL;
  pf->pool = Pool;
  pf->common = pfcom;
  pf->next = NULL;
  P->Funcs = pf;
  P->Handle = P->Funcs->pool->handle;
  DRHOOK_END(0);
  return P;
}

/* *************** End of TABLE "desc" *************** */
