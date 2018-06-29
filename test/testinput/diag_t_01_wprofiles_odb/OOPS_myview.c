#define IS_a_VIEW 1
/* Compilation options used :

	 -V
	 -O3
	 -I/data/users/frwd/installs/odb/gnu/develop/include
	 -lOOPS
	 -DOOPS
	 -s
	 -S
	 -C
	 -lOOPS
	 -DOOPS
	 -w

*/

#include "OOPS.h"


static const char *Sql[] = {
  "select * from desc,hdr",
  ";",
  "",
  NULL
};



#define ODB_CONSIDER_TABLES "/desc/hdr/"





#if !defined(K0_lo_var) && !defined(K0_lo_const)
#define K0_lo 0
#endif
#define desc_ROW ((double)(K0 + 1))
#define hdr_ROW ((double)(K1 + 1))

/* *************** VIEW "myview" *************** */

typedef struct {
  int Handle;
  int PoolNo;
  ODB_Funcs *Funcs;
  int Ncols;
  int Nrows;
  int  USD_symbols;
  int  Replicate_PE;
  int  Npes;
  int *NrowVec;
  int *NrowOffset;
  int  NSortKeys;
  int *SortKeys;
  ODBMAC_VIEW_TABLEDECL_WITH_INDEX(desc);
  ODBMAC_VIEW_TABLEDECL_WITH_INDEX(hdr);
  uint can_UPDATE[RNDUP_DIV(11,MAXBITS)];
} VIEW_myview;

PRIVATE int nV_myview_TAG = 11;
PRIVATE int nV_myview_MEM = 0;

PRIVATE const ODB_Tags V_myview_TAG[11] = {
  /* === SELECT-symbols (count = 11) === */
  { "yyyymmdd:andate@desc", 0, 0, NULL } ,
  { "hhmmss:antime@desc", 0, 0, NULL } ,
  { "linkoffset_t:LINKOFFSET(hdr)@desc", 0, 0, NULL } ,
  { "linklen_t:LINKLEN(hdr)@desc", 0, 0, NULL } ,
  { "pk1int:seqno@hdr", 0, 0, NULL } ,
  { "yyyymmdd:date@hdr", 0, 0, NULL } ,
  { "hhmmss:time@hdr", 0, 0, NULL } ,
  { "pk9real:lat@hdr", 0, 0, NULL } ,
  { "pk9real:lon@hdr", 0, 0, NULL } ,
  { "linkoffset_t:LINKOFFSET(body)@hdr", 0, 0, NULL } ,
  { "linklen_t:LINKLEN(body)@hdr", 0, 0, NULL } ,
  /* === Symbols for auxiliary columns (count = 0) === */
  /* === Symbols in SELECT-expressions (count = 0) === */
  /* === WHERE-symbols (count = 0) === */
  /* === ORDERBY-symbols (count = 0) === */
  /* === UNIQUEBY-symbols (count = 0) === */
};

PRIVATE int nV_myview_PREPTAG = 3;
PRIVATE const ODB_PrepTags V_myview_PREPTAG[3] = {
  /* Prepared tags for faster codb_getnames() */
  { (preptag_name | preptag_extname), 147,
    ";andate@desc;antime@desc;LINKOFFSET(hdr)@desc;LINKLEN(hdr)@desc;"
    "seqno@hdr;date@hdr;time@hdr;lat@hdr;lon@hdr;LINKOFFSET(body)@hdr;"
    "LINKLEN(body)@hdr;" },
  { (preptag_type | preptag_exttype), 102,
    ";yyyymmdd;hhmmss;linkoffset_t;linklen_t;pk1int;yyyymmdd;"
    "hhmmss;pk9real;pk9real;linkoffset_t;linklen_t;" },
  {  preptag_tblname, 12,
    ";@desc;@hdr;" },
};

PRIVATE void
Ccl_V_myview(void *T)
{
  ODBMAC_CCL_V_PRE(myview);
  FREEINDEX(desc);
  FREEINDEX(hdr);
}

PRIVATE int
PrS_V_myview(FILE *do_trace,
  VIEW_myview *P, int it, ODB_PE_Info *PEinfo
  /* TABLE 'desc' */ , int N0, unsigned int BmapIdx0[]
  /* TABLE 'hdr' */ , int N1, unsigned int BmapIdx1[])
{
  double *Addr = (PEinfo && P->USD_symbols > 0) ? PEinfo->addr : NULL;
  int PE, PEstart = 1;
  int PEend = PEinfo ? PEinfo->npes : PEstart;
  int NPEs = PEend - PEstart + 1;
  int K0;
#if defined(K0_lo_var)
  int K0_lo =  0;
#elif defined(K0_lo_const)
  const int K0_lo =  0;
#endif
  int K0_hi = N0;
  int Count = 0;
  linkoffset_t *Phdr_off = UseDSlong(P->T_desc, OOPS, linkoffset_t, P->T_desc->LINKOFFSET(hdr));
  linklen_t *Phdr_len = UseDSlong(P->T_desc, OOPS, linklen_t, P->T_desc->LINKLEN(hdr));
  DRHOOK_START(PrS_V_myview);
  ODBMAC_PEINFO_SETUP();
  for (PE=PEstart; PE<=PEend; PE++) {
    int tmpcount = 0;
    if (Addr) {
      boolean Addr_trigger = 0;
      *Addr = PE;
      if (!Addr_trigger) Addr = NULL;
    }
    ODBMAC_PEINFO_SKIP();
    for (K0=K0_lo; K0<K0_hi; K0++) { /* TABLE 'desc' : weight = 0.000000 */
      int K1_lo = Phdr_off[K0];
      int K1_hi = K1_lo + Phdr_len[K0];
      int K1;
      { /* if-block start */
        tmpcount += Phdr_len[K0]; /* TABLE 'hdr' : weight = 1.000001 */
      } /* if-block end */
    } /* TABLE 'desc' */
    ODBMAC_PEINFO_UPDATE_COUNTS();
    Count += tmpcount;
  } /* for (PE=PEstart; PE<=PEend; PE++) */
  ODBMAC_PEINFO_COPY();
  DRHOOK_END(Count);
  return Count;
}

PRIVATE int
PoS_V_myview(FILE *do_trace,
  const VIEW_myview *P, int it, ODB_PE_Info *PEinfo
  /* TABLE 'desc' */ , int N0, const unsigned int BmapIdx0[], int Index_desc[]
  /* TABLE 'hdr' */ , int N1, const unsigned int BmapIdx1[], int Index_hdr[])
{
  double *Addr = (PEinfo && P->USD_symbols > 0) ? PEinfo->addr : NULL;
  int PE, PEstart = 1;
  int PEend = PEinfo ? PEinfo->npes : PEstart;
  int NPEs = PEend - PEstart + 1;
  int K0;
#if defined(K0_lo_var)
  int K0_lo = 0;
#elif defined(K0_lo_const)
  const int K0_lo = 0;
#endif
  int K0_hi = N0;
  int Count = 0;
  int tmpcount = 0;
  linkoffset_t *Phdr_off = UseDSlong(P->T_desc, OOPS, linkoffset_t, P->T_desc->LINKOFFSET(hdr));
  linklen_t *Phdr_len = UseDSlong(P->T_desc, OOPS, linklen_t, P->T_desc->LINKLEN(hdr));
  DRHOOK_START(PoS_V_myview);
  for (PE=PEstart; PE<=PEend; PE++) {
    if (Addr) {
      boolean Addr_trigger = 0;
      *Addr = PE;
      if (!Addr_trigger) Addr = NULL;
    }
    ODBMAC_PEINFO_BREAKLOOP();
    for (K0=K0_lo; K0<K0_hi; K0++) { /* TABLE 'desc' : weight = 0.000000 */
      int K1_lo = Phdr_off[K0];
      int K1_hi = K1_lo + Phdr_len[K0];
      int K1 = K1_lo;
      { /* if-block start */
        for (K1=K1_lo; K1<K1_hi; K1++) { /* TABLE 'hdr' : weight = 1.000001 */
          Index_desc[tmpcount] = K0;
          Index_hdr[tmpcount] = K1;
          tmpcount++;
        } /* TABLE 'hdr' */
      } /* if-block end */
    } /* TABLE 'desc' */
  } /* for (PE=PEstart; PE<=PEend; PE++) */
  DRHOOK_END(tmpcount);
  return tmpcount;
}

PRIVATE int
Sel_V_myview(void *V, ODB_PE_Info *PEinfo, int phase, void *feedback)
{
  VIEW_myview *P = V;
  int CountPrS = 0;
  int CountPoS = 0;
  int Nbytes = 0;
  ODBMAC_TRACE_SELVIEW_SETUP(myview, "desc,hdr");
  int it = get_thread_id_();
  DRHOOK_START(Sel_V_myview);
  ODBMAC_PEINFO_SELVIEW_SETUP();
  FREEINDEX(desc);
  FREEINDEX(hdr);
  ODBMAC_TRACE_SELVIEW_PRE();
  ODBMAC_VIEW_DELAYED_LOAD(desc);
  ODBMAC_VIEW_DELAYED_LOAD(hdr);
  ODBMAC_TRACE_SELVIEW_0();
  ODBMAC_TRACE_SELVIEW_1();
  { /* Start of pre- & post-select block */
    ALLOCBITMAPINDEX(desc);
    ALLOCBITMAPINDEX(hdr);
    CountPrS = PrS_V_myview(do_trace, P, it, PEinfo
      , P->T_desc->Nrows, P->BitmapIndex_desc
      , P->T_hdr->Nrows, P->BitmapIndex_hdr);
    ODBMAC_TRACE_SELVIEW_POST();
    ALLOCINDEX(desc, CountPrS);
    ALLOCINDEX(hdr, CountPrS);
    ODBMAC_TRACE_SELVIEW_2();
    if (CountPrS > 0) {
      CountPoS = PoS_V_myview(do_trace, P, it, PEinfo
        , P->T_desc->Nrows, P->BitmapIndex_desc, P->Index_desc
        , P->T_hdr->Nrows, P->BitmapIndex_hdr, P->Index_hdr); }
    else { CountPoS = CountPrS; }
    FREEBITMAPINDEX(desc);
    FREEBITMAPINDEX(hdr);
  } /* End of pre-& post-select block */
  ODBMAC_TRACE_SELVIEW_LAST();
  ODBMAC_ERRMSG_SELVIEW(myview);
  ODB_debug_print_index(stdout, "myview", P->PoolNo, CountPrS, 2
        , "desc", P->Index_desc, P->T_desc, P->T_desc->Nrows
        , "hdr", P->Index_hdr, P->T_hdr, P->T_hdr->Nrows);
  P->Nrows = CountPrS;
  DRHOOK_END(CountPrS);
  return CountPrS;
}

PRIVATE int
dGet_V_myview(void *V, double D[],
  int LdimD, int Nrows, int Ncols,
  int ProcID, const int Flag[], int row_offset)
{
  VIEW_myview *P = V;
  int Count = MIN(Nrows, P->Nrows);
  int K1 = 0, K2 = Count;
  int Npes = P->Npes;
  FILE *do_trace = ODB_trace_fp();
  DRHOOK_START(dGet_V_myview);
  ODBMAC_PEINFO_OFFSET();
  Call_GatherGet_VIEW(OOPS, d, 1, myview, yyyymmdd, K1, K2, desc, D, andate, DATATYPE_YYYYMMDD, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 2, myview, hhmmss, K1, K2, desc, D, antime, DATATYPE_HHMMSS, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 3, myview, linkoffset_t, K1, K2, desc, D, LINKOFFSET(hdr), DATATYPE_LINKOFFSET, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 4, myview, linklen_t, K1, K2, desc, D, LINKLEN(hdr), DATATYPE_LINKLEN, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 5, myview, pk1int, K1, K2, hdr, D, seqno, DATATYPE_INT4, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 6, myview, yyyymmdd, K1, K2, hdr, D, date, DATATYPE_YYYYMMDD, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 7, myview, hhmmss, K1, K2, hdr, D, time, DATATYPE_HHMMSS, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 8, myview, pk9real, K1, K2, hdr, D, lat, DATATYPE_REAL8, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 9, myview, pk9real, K1, K2, hdr, D, lon, DATATYPE_REAL8, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 10, myview, linkoffset_t, K1, K2, hdr, D, LINKOFFSET(body), DATATYPE_LINKOFFSET, 0, 0);
  Call_GatherGet_VIEW(OOPS, d, 11, myview, linklen_t, K1, K2, hdr, D, LINKLEN(body), DATATYPE_LINKLEN, 0, 0);
  DRHOOK_END(K2-K1);
  return K2-K1;
}

PRIVATE void
Dim_V_myview(void *V, int *Nrows, int *Ncols, int *Nrowoffset, int ProcID) { ODBMAC_DIM(myview); }

#define Swapout_V_myview Ccl_V_myview

PRIVATE int
Sql_V_myview(FILE *fp, int mode, const char *prefix, const char *postfix, char **sqlout) { ODBMAC_VIEWSQL(); }

PRIVATE int
ColAux_V_myview(void *V, int colaux[], int colaux_len)
{
  int filled = 0;
  static const int ColAux_len = 11;
  static const int ColAux[11] = {1,2,3,4,5,6,7,8,9,10,11};
  ODBMAC_COPY_COLAUX(myview);
  return filled;
}

PRIVATE int *
SortKeys_V_myview(void *V, int *NSortKeys) { ODBMAC_SORTKEYS(myview); }

PRIVATE int 
UpdateInfo_V_myview(void *V, const int ncols, int can_UPDATE[]) { ODBMAC_UPDATEINFO(myview); }

PRIVATE int *
GetIndex_V_myview(void *V, const char *Table, int *Nidx)
{
  VIEW_myview *P = V;
  int Dummy = 0;
  int *Nlen = Nidx ? Nidx : &Dummy;
  ODBMAC_GETINDEX(desc);
  ODBMAC_GETINDEX(hdr);
  return NULL;
}

PRIVATE int
PutIndex_V_myview(void *V, const char *Table, int Nidx, int idx[], int by_address)
{
  /* *** Warning: This is a VERY DANGEROUS routine -- if misused !!! */
  VIEW_myview *P = V;
  int rc = 0;
  ODBMAC_PUTINDEX(desc);
  ODBMAC_PUTINDEX(hdr);
  return rc;
}

PRIVATE void
PEinfo_V_myview(void *V, ODB_PE_Info *PEinfo) { ODBMAC_PEINFO_INIT(myview); }

PRIVATE void *
Init_V_myview(void *V, ODB_Pool *Pool, int Dummy1, int Dummy2, int it, int add_vars)
{
  VIEW_myview *P = V;
  int PoolNo = Pool->poolno;
  ODB_Funcs *pf;
  static ODB_CommonFuncs *pfcom = NULL; /* Shared between pools & threads */
  DRHOOK_START(Init_V_myview);
  if (!P) ALLOC(P, 1);
  P->PoolNo = PoolNo;
  P->Ncols = 11;
  P->Nrows = 0;
  P->USD_symbols = 0; /* In SELECT = 0 ; In WHERE = 0 */
  P->Replicate_PE = 0;
  P->Npes = 0;
  P->NrowVec = NULL;
  P->NrowOffset = NULL;
  P->NSortKeys = 0;
  P->SortKeys = NULL;
  /* Initially all bits set to 0, which corresponds to read-only -mode for all */
  memset(P->can_UPDATE, 0, sizeof(P->can_UPDATE));
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 0, 0); /* yyyymmdd:andate@desc */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 1, 1); /* hhmmss:antime@desc */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 2, 2); /* linkoffset_t:LINKOFFSET(hdr)@desc */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 3, 3); /* linklen_t:LINKLEN(hdr)@desc */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 4, 4); /* pk1int:seqno@hdr */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 5, 5); /* yyyymmdd:date@hdr */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 6, 6); /* hhmmss:time@hdr */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 7, 7); /* pk9real:lat@hdr */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 8, 8); /* pk9real:lon@hdr */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 9, 9); /* linkoffset_t:LINKOFFSET(body)@hdr */
  ODBIT_unset(P->can_UPDATE, P->Ncols, MAXBITS, 10, 10); /* linklen_t:LINKLEN(body)@hdr */
  {
    ODB_Pool *p = Pool;
    ODBMAC_ASSIGN_TABLEDATA(desc);
    ODBMAC_ASSIGN_TABLEDATA(hdr);
  }
  NULLIFY_INDEX(desc);
  NULLIFY_INDEX(hdr);
  if (!pfcom) { /* Initialize once only */
    CALLOC(pfcom,1);
    { static char s[] = "myview"; pfcom->name = s; }
    pfcom->is_table = 0;
    pfcom->is_considered = 0;
    pfcom->ntables = 2;
    pfcom->ncols = P->Ncols;
    pfcom->tableno = 0;
    pfcom->rank = 0;
    pfcom->wt = 0;
    pfcom->tags = V_myview_TAG;
    pfcom->preptags = V_myview_PREPTAG;
    pfcom->ntag = nV_myview_TAG;
    pfcom->npreptag = nV_myview_PREPTAG;
    pfcom->nmem = nV_myview_MEM;
    pfcom->Info = NULL;
    pfcom->create_index = 0;
    pfcom->init = Init_V_myview;
    pfcom->swapout = Swapout_V_myview;
    pfcom->dim = Dim_V_myview;
    pfcom->sortkeys = SortKeys_V_myview;
    pfcom->update_info = UpdateInfo_V_myview;
    pfcom->aggr_info = NULL;
    pfcom->getindex = GetIndex_V_myview;
    pfcom->putindex = PutIndex_V_myview;
    pfcom->peinfo = PEinfo_V_myview;
    pfcom->select = Sel_V_myview;
    pfcom->remove = NULL;
    pfcom->cancel = Ccl_V_myview;
    pfcom->dget = dGet_V_myview;
    pfcom->dput = NULL; /* All view entries read-only */
    pfcom->load = NULL;
    pfcom->store = NULL;
    pfcom->pack = NULL;
    pfcom->unpack = NULL;
    pfcom->sql = Sql_V_myview;
    pfcom->ncols_aux = 0;
    pfcom->colaux = ColAux_V_myview;
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
  if (add_vars) {
  } /* if (add_vars) */
  DRHOOK_END(0);
  return P;
}

/* *************** End of VIEW "myview" *************** */

PUBLIC ODB_Funcs *
Anchor2OOPS_myview(void *V, ODB_Pool *pool, int *nviews, int it, int add_vars)
{
  VIEW_myview *P = V;
  ODB_Funcs *pf;
  DRHOOK_START(Anchor2OOPS_myview);
  if (!P) P = Init_V_myview(NULL, pool, -1, -1, it, add_vars);
  if (nviews) *nviews = 1;
  pf = P->Funcs;
  DRHOOK_END(0);
  return pf;
}
