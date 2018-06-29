#ifndef ODB_GENCODE
#define ODB_GENCODE 1
#endif


/* Software revision : CY43R0.000 (430000) */

#include "odb.h"
#include "odb_macros.h"
#include "cdrhook.h"

#define ODB_LABEL    "OOPS"


/* Compilation options used :

	 -V
	 -O3
	 -I/data/users/frwd/installs/odb/gnu/develop/include
	 -lOOPS
	 -DOOPS
	 -s
	 -S
	 -C

*/

/* ----- Table hierarchy (= the default scanning order) : # of tables = 3

       Rank#         Table :     Order#   Weight
       -----         ----- :     ------   ------
           0          desc :          0   0.000000
           1           hdr :          1   1.000001
           2          body :          2   1.000002

   ----- End of table hierarchy ----- */

PUBLIC void OOPS_print_flags_file(void);

#if defined(ODB_MAINCODE)

PUBLIC double USD_mdi_OOPS = 2147483647; /* $mdi */

#endif /* defined(ODB_MAINCODE) */

DefineDS(yyyymmdd);
#define OOPS_pack_INT ODB_pack_INT
#define OOPS_unpack_INT ODB_unpack_INT
DS_Unpacking(OOPS, INT, yyyymmdd)
DS_Packing(OOPS, INT, yyyymmdd)

DefineDS(hhmmss);
DS_Unpacking(OOPS, INT, hhmmss)
DS_Packing(OOPS, INT, hhmmss)

DefineDS(linkoffset_t);
DS_Unpacking(OOPS, INT, linkoffset_t)
DS_Packing(OOPS, INT, linkoffset_t)

DefineDS(linklen_t);
DS_Unpacking(OOPS, INT, linklen_t)
DS_Packing(OOPS, INT, linklen_t)

DefineDS(pk1int);
DS_Unpacking(OOPS, INT, pk1int)
DS_Packing(OOPS, INT, pk1int)

DefineDS(pk9real);
#define OOPS_pack_DBL ODB_pack_DBL
#define OOPS_unpack_DBL ODB_unpack_DBL
DS_Unpacking(OOPS, DBL, pk9real)
DS_Packing(OOPS, DBL, pk9real)


#if defined(IS_a_TABLE_desc) || defined(ODB_MAINCODE) || defined(IS_a_VIEW)

/* *************** TABLE "desc" : appearance order#0, hierarchy rank# 0, weight = 0.000000 *************** */

typedef struct {
  int Handle;
  int PoolNo;
  ODB_Funcs *Funcs;
  boolean Is_loaded;
  boolean Is_new;
  boolean Swapped_out;
  boolean Byteswap;
  int IO_method;
  int Created[2];
  int LastUpdated[2];
  int Ncols;
  int Nrows;
  int Nalloc;
  int Numreqs;
  DeclareDS(yyyymmdd,andate);
  DeclareDS(hhmmss,antime);
  DeclareDS(linkoffset_t,LINKOFFSET(hdr));
  DeclareDS(linklen_t,LINKLEN(hdr));
} TABLE_desc;

#endif /* defined(IS_a_TABLE_desc) || defined(ODB_MAINCODE)  || defined(IS_a_VIEW) */

#if !defined(ODB_MAINCODE) && defined(IS_a_TABLE_desc)
extern const ODB_Tags *OOPS_Set_T_desc_TAG(int *ntag_out, int *nmem_out);
extern const ODB_PrepTags *OOPS_Set_T_desc_PREPTAG(int *npreptag_out);
#elif defined(ODB_MAINCODE)
PRIVATE const ODB_Tags *OOPS_T_desc_TAG = NULL;
PRIVATE const ODB_PrepTags *OOPS_T_desc_PREPTAG = NULL;
PRIVATE int OOPS_nT_desc_TAG = 0;
PRIVATE int OOPS_nT_desc_PREPTAG = 0;
PRIVATE int OOPS_nT_desc_MEM = 0;
PUBLIC const ODB_Tags *
OOPS_Set_T_desc_TAG(int *ntag_out, int *nmem_out)
{
  if (!OOPS_T_desc_TAG) {
    int ntag = 4;
    ODB_Tags *T = NULL;
    CALLOC(T, ntag);
    { static char s[] = "yyyymmdd:andate@desc"; T[0].name = s; }
    { static char s[] = "hhmmss:antime@desc"; T[1].name = s; }
    { static char s[] = "linkoffset_t:LINKOFFSET(hdr)@desc"; T[2].name = s; }
    { static char s[] = "linklen_t:LINKLEN(hdr)@desc"; T[3].name = s; }
    OOPS_T_desc_TAG = T;
    OOPS_nT_desc_TAG = ntag;
    OOPS_nT_desc_MEM = 0;
  }
  if (ntag_out) *ntag_out = OOPS_nT_desc_TAG;
  if (nmem_out) *nmem_out = OOPS_nT_desc_MEM;
  return OOPS_T_desc_TAG;
}
PUBLIC const ODB_PrepTags *
OOPS_Set_T_desc_PREPTAG(int *npreptag_out)
{
  if (!OOPS_T_desc_PREPTAG) {
    int npreptag = 2;
    ODB_PrepTags *T = NULL;
    ALLOC(T, npreptag);
    T[0].tagtype = (preptag_name | preptag_extname);
    T[0].longname_len = 64;
    { static char s[] =
      ";andate@desc;antime@desc;LINKOFFSET(hdr)@desc;LINKLEN(hdr)@desc;";
      T[0].longname = s; }
    T[1].tagtype = (preptag_type | preptag_exttype);
    T[1].longname_len = 40;
    { static char s[] =
      ";yyyymmdd;hhmmss;linkoffset_t;linklen_t;";
      T[1].longname = s; }
    OOPS_T_desc_PREPTAG = T;
    OOPS_nT_desc_PREPTAG = npreptag;
  }
  if (npreptag_out) *npreptag_out = OOPS_nT_desc_PREPTAG;
  return OOPS_T_desc_PREPTAG;
}
#endif

#if defined(ODB_MAINCODE)

extern int OOPS_Pack_T_desc(void *T);
extern int OOPS_Unpack_T_desc(void *T);
extern int OOPS_Sel_T_desc(void *T, ODB_PE_Info *PEinfo, int phase, void *feedback);
PreGetTable(OOPS, d, double, desc);
PrePutTable(OOPS, d, double, desc);
PreLoadTable(OOPS, desc);
PreStoreTable(OOPS, desc);
extern void OOPS_Dim_T_desc(void *T, int *Nrows, int *Ncols, int *Nrowoffset, int ProcID);
extern void OOPS_Swapout_T_desc(void *T);
extern int OOPS_Sql_T_desc(FILE *fp, int mode, const char *prefix, const char *postfix, char **sqlout);
extern void *OOPS_Init_T_desc(void *T, ODB_Pool *Pool, int Is_new, int IO_method, int it, int dummy);

#endif /* defined(ODB_MAINCODE) */

#if defined(IS_a_TABLE_hdr) || defined(ODB_MAINCODE) || defined(IS_a_VIEW)

/* *************** TABLE "hdr" : appearance order#1, hierarchy rank# 1, weight = 1.000001 *************** */

typedef struct {
  int Handle;
  int PoolNo;
  ODB_Funcs *Funcs;
  boolean Is_loaded;
  boolean Is_new;
  boolean Swapped_out;
  boolean Byteswap;
  int IO_method;
  int Created[2];
  int LastUpdated[2];
  int Ncols;
  int Nrows;
  int Nalloc;
  int Numreqs;
  DeclareDS(pk1int,seqno);
  DeclareDS(yyyymmdd,date);
  DeclareDS(hhmmss,time);
  DeclareDS(pk9real,lat);
  DeclareDS(pk9real,lon);
  DeclareDS(linkoffset_t,LINKOFFSET(body));
  DeclareDS(linklen_t,LINKLEN(body));
} TABLE_hdr;

#endif /* defined(IS_a_TABLE_hdr) || defined(ODB_MAINCODE)  || defined(IS_a_VIEW) */

#if !defined(ODB_MAINCODE) && defined(IS_a_TABLE_hdr)
extern const ODB_Tags *OOPS_Set_T_hdr_TAG(int *ntag_out, int *nmem_out);
extern const ODB_PrepTags *OOPS_Set_T_hdr_PREPTAG(int *npreptag_out);
#elif defined(ODB_MAINCODE)
PRIVATE const ODB_Tags *OOPS_T_hdr_TAG = NULL;
PRIVATE const ODB_PrepTags *OOPS_T_hdr_PREPTAG = NULL;
PRIVATE int OOPS_nT_hdr_TAG = 0;
PRIVATE int OOPS_nT_hdr_PREPTAG = 0;
PRIVATE int OOPS_nT_hdr_MEM = 0;
PUBLIC const ODB_Tags *
OOPS_Set_T_hdr_TAG(int *ntag_out, int *nmem_out)
{
  if (!OOPS_T_hdr_TAG) {
    int ntag = 7;
    ODB_Tags *T = NULL;
    CALLOC(T, ntag);
    { static char s[] = "pk1int:seqno@hdr"; T[0].name = s; }
    { static char s[] = "yyyymmdd:date@hdr"; T[1].name = s; }
    { static char s[] = "hhmmss:time@hdr"; T[2].name = s; }
    { static char s[] = "pk9real:lat@hdr"; T[3].name = s; }
    { static char s[] = "pk9real:lon@hdr"; T[4].name = s; }
    { static char s[] = "linkoffset_t:LINKOFFSET(body)@hdr"; T[5].name = s; }
    { static char s[] = "linklen_t:LINKLEN(body)@hdr"; T[6].name = s; }
    OOPS_T_hdr_TAG = T;
    OOPS_nT_hdr_TAG = ntag;
    OOPS_nT_hdr_MEM = 0;
  }
  if (ntag_out) *ntag_out = OOPS_nT_hdr_TAG;
  if (nmem_out) *nmem_out = OOPS_nT_hdr_MEM;
  return OOPS_T_hdr_TAG;
}
PUBLIC const ODB_PrepTags *
OOPS_Set_T_hdr_PREPTAG(int *npreptag_out)
{
  if (!OOPS_T_hdr_PREPTAG) {
    int npreptag = 2;
    ODB_PrepTags *T = NULL;
    ALLOC(T, npreptag);
    T[0].tagtype = (preptag_name | preptag_extname);
    T[0].longname_len = 84;
    { static char s[] =
      ";seqno@hdr;date@hdr;time@hdr;lat@hdr;lon@hdr;LINKOFFSET(body)@hdr;"
      "LINKLEN(body)@hdr;";
      T[0].longname = s; }
    T[1].tagtype = (preptag_type | preptag_exttype);
    T[1].longname_len = 63;
    { static char s[] =
      ";pk1int;yyyymmdd;hhmmss;pk9real;pk9real;linkoffset_t;"
      "linklen_t;";
      T[1].longname = s; }
    OOPS_T_hdr_PREPTAG = T;
    OOPS_nT_hdr_PREPTAG = npreptag;
  }
  if (npreptag_out) *npreptag_out = OOPS_nT_hdr_PREPTAG;
  return OOPS_T_hdr_PREPTAG;
}
#endif

#if defined(ODB_MAINCODE)

extern int OOPS_Pack_T_hdr(void *T);
extern int OOPS_Unpack_T_hdr(void *T);
extern int OOPS_Sel_T_hdr(void *T, ODB_PE_Info *PEinfo, int phase, void *feedback);
PreGetTable(OOPS, d, double, hdr);
PrePutTable(OOPS, d, double, hdr);
PreLoadTable(OOPS, hdr);
PreStoreTable(OOPS, hdr);
extern void OOPS_Dim_T_hdr(void *T, int *Nrows, int *Ncols, int *Nrowoffset, int ProcID);
extern void OOPS_Swapout_T_hdr(void *T);
extern int OOPS_Sql_T_hdr(FILE *fp, int mode, const char *prefix, const char *postfix, char **sqlout);
extern void *OOPS_Init_T_hdr(void *T, ODB_Pool *Pool, int Is_new, int IO_method, int it, int dummy);

#endif /* defined(ODB_MAINCODE) */

#if defined(IS_a_TABLE_body) || defined(ODB_MAINCODE) || defined(IS_a_VIEW)

/* *************** TABLE "body" : appearance order#2, hierarchy rank# 2, weight = 1.000002 *************** */

typedef struct {
  int Handle;
  int PoolNo;
  ODB_Funcs *Funcs;
  boolean Is_loaded;
  boolean Is_new;
  boolean Swapped_out;
  boolean Byteswap;
  int IO_method;
  int Created[2];
  int LastUpdated[2];
  int Ncols;
  int Nrows;
  int Nalloc;
  int Numreqs;
  DeclareDS(pk1int,varno);
  DeclareDS(pk9real,obsvalue);
  DeclareDS(pk1int,entryno);
  DeclareDS(pk9real,vertco_reference_1);
} TABLE_body;

#endif /* defined(IS_a_TABLE_body) || defined(ODB_MAINCODE)  || defined(IS_a_VIEW) */

#if !defined(ODB_MAINCODE) && defined(IS_a_TABLE_body)
extern const ODB_Tags *OOPS_Set_T_body_TAG(int *ntag_out, int *nmem_out);
extern const ODB_PrepTags *OOPS_Set_T_body_PREPTAG(int *npreptag_out);
#elif defined(ODB_MAINCODE)
PRIVATE const ODB_Tags *OOPS_T_body_TAG = NULL;
PRIVATE const ODB_PrepTags *OOPS_T_body_PREPTAG = NULL;
PRIVATE int OOPS_nT_body_TAG = 0;
PRIVATE int OOPS_nT_body_PREPTAG = 0;
PRIVATE int OOPS_nT_body_MEM = 0;
PUBLIC const ODB_Tags *
OOPS_Set_T_body_TAG(int *ntag_out, int *nmem_out)
{
  if (!OOPS_T_body_TAG) {
    int ntag = 4;
    ODB_Tags *T = NULL;
    CALLOC(T, ntag);
    { static char s[] = "pk1int:varno@body"; T[0].name = s; }
    { static char s[] = "pk9real:obsvalue@body"; T[1].name = s; }
    { static char s[] = "pk1int:entryno@body"; T[2].name = s; }
    { static char s[] = "pk9real:vertco_reference_1@body"; T[3].name = s; }
    OOPS_T_body_TAG = T;
    OOPS_nT_body_TAG = ntag;
    OOPS_nT_body_MEM = 0;
  }
  if (ntag_out) *ntag_out = OOPS_nT_body_TAG;
  if (nmem_out) *nmem_out = OOPS_nT_body_MEM;
  return OOPS_T_body_TAG;
}
PUBLIC const ODB_PrepTags *
OOPS_Set_T_body_PREPTAG(int *npreptag_out)
{
  if (!OOPS_T_body_PREPTAG) {
    int npreptag = 2;
    ODB_PrepTags *T = NULL;
    ALLOC(T, npreptag);
    T[0].tagtype = (preptag_name | preptag_extname);
    T[0].longname_len = 63;
    { static char s[] =
      ";varno@body;obsvalue@body;entryno@body;vertco_reference_1@body;";
      T[0].longname = s; }
    T[1].tagtype = (preptag_type | preptag_exttype);
    T[1].longname_len = 31;
    { static char s[] =
      ";pk1int;pk9real;pk1int;pk9real;";
      T[1].longname = s; }
    OOPS_T_body_PREPTAG = T;
    OOPS_nT_body_PREPTAG = npreptag;
  }
  if (npreptag_out) *npreptag_out = OOPS_nT_body_PREPTAG;
  return OOPS_T_body_PREPTAG;
}
#endif

#if defined(ODB_MAINCODE)

extern int OOPS_Pack_T_body(void *T);
extern int OOPS_Unpack_T_body(void *T);
extern int OOPS_Sel_T_body(void *T, ODB_PE_Info *PEinfo, int phase, void *feedback);
PreGetTable(OOPS, d, double, body);
PrePutTable(OOPS, d, double, body);
PreLoadTable(OOPS, body);
PreStoreTable(OOPS, body);
extern void OOPS_Dim_T_body(void *T, int *Nrows, int *Ncols, int *Nrowoffset, int ProcID);
extern void OOPS_Swapout_T_body(void *T);
extern int OOPS_Sql_T_body(FILE *fp, int mode, const char *prefix, const char *postfix, char **sqlout);
extern void *OOPS_Init_T_body(void *T, ODB_Pool *Pool, int Is_new, int IO_method, int it, int dummy);

#endif /* defined(ODB_MAINCODE) */

