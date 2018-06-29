#undef ODB_MAINCODE
#undef IS_a_VIEW
#define IS_a_TABLE_body 1
#include "OOPS.h"

extern double USD_mdi_OOPS; /* $mdi */


PUBLIC int
OOPS_Pack_T_body(void *T)
{
  int Nbytes = 0;
  TABLE_body *P = T;
  Packed_DS *PDS;
  if (P->Is_loaded) {
    PDS = PackDS(P, OOPS, pk1int, varno); CHECK_PDS_ERROR(1);
    PDS = PackDS(P, OOPS, pk9real, obsvalue); CHECK_PDS_ERROR(2);
    PDS = PackDS(P, OOPS, pk1int, entryno); CHECK_PDS_ERROR(3);
    PDS = PackDS(P, OOPS, pk9real, vertco_reference_1); CHECK_PDS_ERROR(4);
  }
  return Nbytes;
}

PUBLIC int
OOPS_Unpack_T_body(void *T)
{
  int Nbytes = 0;
  TABLE_body *P = T;
  if (P->Is_loaded) {
    UseDS(P, OOPS, pk1int, varno); Nbytes += BYTESIZE(P->varno.d);
    UseDS(P, OOPS, pk9real, obsvalue); Nbytes += BYTESIZE(P->obsvalue.d);
    UseDS(P, OOPS, pk1int, entryno); Nbytes += BYTESIZE(P->entryno.d);
    UseDS(P, OOPS, pk9real, vertco_reference_1); Nbytes += BYTESIZE(P->vertco_reference_1.d);
  }
  return Nbytes;
}

PUBLIC int
OOPS_Sel_T_body(void *T, ODB_PE_Info *PEinfo, int phase, void *feedback)
{
  TABLE_body *P = T;
  ODBMAC_TABLE_DELAYED_LOAD(body);
  return P->Nrows;
}


PreGetTable(OOPS, d, double, body)
  Call_CopyGet_TABLE(OOPS, d, 1, body, pk1int, D, varno, Count, DATATYPE_INT4);
  Call_CopyGet_TABLE(OOPS, d, 2, body, pk9real, D, obsvalue, Count, DATATYPE_REAL8);
  Call_CopyGet_TABLE(OOPS, d, 3, body, pk1int, D, entryno, Count, DATATYPE_INT4);
  Call_CopyGet_TABLE(OOPS, d, 4, body, pk9real, D, vertco_reference_1, Count, DATATYPE_REAL8);
PostGetTable(d, double, body)


PrePutTable(OOPS, d, double, body)
  Call_CopyPut_TABLE(OOPS, d, 1, body, pk1int, varno, D, Count, DATATYPE_INT4);
  Call_CopyPut_TABLE(OOPS, d, 2, body, pk9real, obsvalue, D, Count, DATATYPE_REAL8);
  Call_CopyPut_TABLE(OOPS, d, 3, body, pk1int, entryno, D, Count, DATATYPE_INT4);
  Call_CopyPut_TABLE(OOPS, d, 4, body, pk9real, vertco_reference_1, D, Count, DATATYPE_REAL8);
PostPutTable(d, double, body)

PreLoadTable(OOPS, body);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, pk1int, DATATYPE_INT4, varno);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, pk9real, DATATYPE_REAL8, obsvalue);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, pk1int, DATATYPE_INT4, entryno);
  Call_Read_DS(OOPS, fp_idx, filename, Nbytes, pk9real, DATATYPE_REAL8, vertco_reference_1);
PostLoadTable(body)

PreStoreTable(OOPS, body);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, pk1int, DATATYPE_INT4, varno);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, pk9real, DATATYPE_REAL8, obsvalue);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, pk1int, DATATYPE_INT4, entryno);
  Call_Write_DS(OOPS, fp_idx, filename, Nbytes, pk9real, DATATYPE_REAL8, vertco_reference_1);
PostStoreTable(body)

DefineLookupTable(body)

PUBLIC void
OOPS_Dim_T_body(void *T, int *Nrows, int *Ncols,
  int *Nrowoffset, int ProcID)
{
  TABLE_body *P = T;
  Call_LookupTable(body, P, Nrows, Ncols);
  if (Nrowoffset) *Nrowoffset = 0;
}

PUBLIC void
OOPS_Swapout_T_body(void *T)
{
  TABLE_body *P = T;
  int Nbytes = 0;
  int Count = 0;
  int PoolNo = P->PoolNo;
  FILE *do_trace = NULL;
  if (P->Swapped_out || !P->Is_loaded) return;
  do_trace = ODB_trace_fp();
  FreeDS(P, varno, Nbytes, Count);
  FreeDS(P, obsvalue, Nbytes, Count);
  FreeDS(P, entryno, Nbytes, Count);
  FreeDS(P, vertco_reference_1, Nbytes, Count);
  P->Nrows = 0;
  P->Nalloc = 0;
  P->Is_loaded = 0;
  P->Swapped_out = P->Is_new ? 0 : 1;
  ODBMAC_TRACE_SWAPOUT(body,4);
}

DefineRemoveTable(OOPS, body)

PUBLIC int
OOPS_Sql_T_body(FILE *fp, int mode, const char *prefix, const char *postfix, char **sqlout) { ODBMAC_TABLESQL(); }

PUBLIC void *
OOPS_Init_T_body(void *T, ODB_Pool *Pool, int Is_new, int IO_method, int it, int dummy)
{
  TABLE_body *P = T;
  int PoolNo = Pool->poolno;
  ODB_Funcs *pf;
  static ODB_CommonFuncs *pfcom = NULL; /* Shared between pools & threads */
  DRHOOK_START(OOPS_Init_T_body);
  if (!P) ALLOC(P, 1);
  PreInitTable(P, 4);
  InitDS(pk1int, DATATYPE_INT4, varno, body, 1);
  InitDS(pk9real, DATATYPE_REAL8, obsvalue, body, 9);
  InitDS(pk1int, DATATYPE_INT4, entryno, body, 1);
  InitDS(pk9real, DATATYPE_REAL8, vertco_reference_1, body, 9);
  if (!pfcom) { /* Initialize once only */
    CALLOC(pfcom,1);
    { static char s[] = "@body"; pfcom->name = s; }
    pfcom->is_table = 1;
    pfcom->is_considered = 0;
    pfcom->ntables = 0;
    pfcom->ncols = 4;
    pfcom->tableno = 2;
    pfcom->rank = 2;
    pfcom->wt = 1.000002;
    pfcom->tags = OOPS_Set_T_body_TAG(&pfcom->ntag, &pfcom->nmem);
    pfcom->preptags = OOPS_Set_T_body_PREPTAG(&pfcom->npreptag);
    pfcom->Info = NULL;
    pfcom->create_index = 0;
    pfcom->init = OOPS_Init_T_body;
    pfcom->swapout = OOPS_Swapout_T_body;
    pfcom->dim = OOPS_Dim_T_body;
    pfcom->sortkeys = NULL;
    pfcom->update_info = NULL;
    pfcom->aggr_info = NULL;
    pfcom->getindex = NULL; /* N/A */
    pfcom->putindex = NULL; /* N/A */
    pfcom->select = OOPS_Sel_T_body;
    pfcom->remove = OOPS_Remove_T_body;
    pfcom->peinfo = NULL; /* N/A */
    pfcom->cancel = NULL;
    pfcom->dget = OOPS_dGet_T_body; /* REAL(8) dbmgr */
    pfcom->dput = OOPS_dPut_T_body; /* REAL(8) dbmgr */
    pfcom->load = OOPS_Load_T_body;
    pfcom->store = OOPS_Store_T_body;
    pfcom->pack = OOPS_Pack_T_body;
    pfcom->unpack = OOPS_Unpack_T_body;
    pfcom->sql = OOPS_Sql_T_body;
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

/* *************** End of TABLE "body" *************** */
