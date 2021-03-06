/******************************************************************************\
UTILITY:    FINDOUT
STRUCTURE:  common CFLOW architecture.
TYPE:       Powerflow (BPF) output report post-processor.
SUMMARY:    Generates a table of outages and corresponding branch overloads or
            bus voltage violations from multiple pfo files. Sorts and screens.
RELATED:    LINEFLOW, MIMIC, CFUSE
SEE ALSO:   TO_DECWRITE.HELP, VMS_TO_PC_TO_EXCEL.HELP 
UPDATED:    April 2, 1997 
LANGUAGE:   Standard C.  CFLOW Library.  cf_util.h 
DEVELOPER:  William D. Rogers, BPA, TOP, 230-3806, wdrogers@bpa.gov
REQUESTER:  IPF User Group (L. Stadler, P. Larguier, K. Kohne) 
USERS:      Kyle Kohne, Larry Stadler, Dorothy Betzing, Kendall Rydell
IPF:        version 317 or above recommended; also works with PTI
PURPOSE:    Works with the .pfo output files of /OUTAGE_SIMULATION runs as a 
            post-processor to screen and sort the results and present them in 
            tabular form.  Tables of 'Outages and Overloads' or 'Outages and 
            Bus Violations' can be produced.  Entries in these tables can be
            screened according to Zone, Owner, Base kV, Loading and Bus Voltage.
            Tables can be sorted by Zone, Owner, Base kV, or alphabetically. The
            idea is to allow the user to automate the creation of a report 
            detailing the results of outages--saving time and reducing errors 
            from the current method of doing so which generally includes cut and
            paste operation with an editor.

            Data fields in the output report table are character delimited to 
            ease importing to MS Excel or DECwrite.
\******************************************************************************/
/******************************* #include *************************************/
#include "cflowlib.h"
#include "cf_util.h"
/***************************** end #include ***********************************/
/******************************* #define **************************************/

#define	 VERSN_NUM	  110
#define  VERSN_DATE	  "06-30-99"
#define  FF_IPF_VERSION   324           /* GPF.EXE_V324 or above recommended */
#define  FF_PAGE_LENGTH   61
#define  DOT_LINE         "..................................................."
#define  FF_WILD          '?'
#define  LOG_SPEC         "findout.log"
#define  OUT_NAME         "findout"
#define  DFLT_CASE_PER_PAGE 2
#define  DFLT_MIN_LOAD_PCT  80.00
#define  DFLT_MAX_DELTA_PCT 0.0
#define  DFLT_MAX_500_PU    1.100
#define  DFLT_MIN_500_PU    1.000
#define  DFLT_MAX_BUS_PU    1.052
#define  DFLT_MIN_BUS_PU    0.950
#define  QUERY_PFO  (int) (1<< 0)  /* prompt for *.pfo files */
#define  QUERY_DATA (int) (1<< 1)  /* prompt for data file listing branch/bus*/
#define  QUERY_SOLN (int) (1<< 2)  /* prompt for including sol'n problem data*/
#define  QUERY_OWNR (int) (1<< 3)  /* prompt for owners of interest */
#define  QUERY_ZONE (int) (1<< 4)  /* prompt for zones of interest */
#define  QUERY_BSKV (int) (1<< 5)  /* prompt for base kV of interest */
#define  QUERY_LOAD (int) (1<< 6)  /* prompt for MAX PCT LOADING */
#define  QUERY_SORT (int) (1<< 7)  /* prompt for sort order */
#define  QUERY_FORM (int) (1<< 8)  /* prompt for report format */
#define  QUERY_NAME (int) (1<< 9)  /* prompt for report name */
#define  QUERY_WDTH (int) (1<<10)  /* prompt for report width in cases/table */
#define  QUERY__TRC (int) (1<<11)  /* prompt for whether or not to make .TRC */
#define  QUERY_CONT (int) (1<<12)  /* prompt for continuation of run */
#define  QUERY_TYPE (int) (1<<13)  /* prompt for continuation of run */
#define  QUERY_VOLT (int) (1<<14)  /* prompt for continuation of run */
#define  QUERY_OUTG (int) (1<<15)  /* prompt for list of branches/busses */
#define  QUERY_COMO (int) (1<<16)  /* prompt for list of branches/busses */
#define  QUERY_OVLD (int) (1<<17)  /* prompt for list of branches/busses */
#define  QUERY_BUSV (int) (1<<18)  /* prompt for list of branches/busses */
#define  QUERY_OW_P (int) (1<<19)  /* prompt for owners of interest */
#define  QUERY_ZN_P (int) (1<<20)  /* prompt for zones of interest */
#define  QUERY_KV_P (int) (1<<21)  /* prompt for base kV of interest */
#define  QUERY_DLTA (int) (1<<22)  /* prompt for max change in load or volt */
#define  QUERY_DIFF (int) (1<<23)  /* prompt for whether to do difference rpt */
#define  QUERY_REDU (int) (1<<24)  /* prompt for removal of redundant outages */
#define  QUERY_VARI (int) (1<<25)  /* prompt for whether to do variance rpt */
#define  QUERY_SPAN (int) (1<<26)  /* prompt for variance span  */
#define  QUERY_SSEP (int) (1<<27)  /* prompt for listing system separations */
#define  READ_INC        (int)    1 /* found /INCLUDE card in CFLOW data file */
#define  READ_PFO        (int)    2 /* found /PFO     card in CFLOW data file */
#define  READ_OUT        (int)    3 /* found /OUTAGE  card in CFLOW data file */
#define  READ_COM        (int)    4 /* found /COMMON_MODE  in CFLOW data file */
#define  READ_OVR        (int)    5 /* found /OVERLOAD     in CFLOW data file */
#define  READ_BUS        (int)    6 /* found /BUS     card in CFLOW data file */
#define  READ_OWN        (int)    8 /* found /OUTG_OWNER   in CFLOW data file */
#define  READ_ZON        (int)    9 /* found /OUTG_ZONE    in CFLOW data file */
#define  READ_BKV        (int)   10 /* found /OUTG_BASE_KV in CFLOW data file */
#define  READ_LIM        (int)   11 /* found /LIMITS  card in CFLOW data file */
#define  READ_REP        (int)   12 /* found /REPORT  card in CFLOW data file */
#define  READ_OWP        (int)   13 /* found /PROB_OWNER   in CFLOW data file */
#define  READ_ZNP        (int)   14 /* found /PROB_ZONE    in CFLOW data file */
#define  READ_KVP        (int)   15 /* found /PROB_BASE_KV in CFLOW data file */

#define  SORT_BY_ALPHA   (int)    1
#define  SORT_BY_BASEKV  (int)    2
#define  SORT_BY_OWNER   (int)    3
#define  SORT_BY_ZONE    (int)    4
#define  SORT_BY_INPUT   (int)    5
#define  SORT_BY_SOLN    (int)    6
#define  SORT_BY_DIFF    (int)    7
#define  VARI_NO         (int)    0
#define  VARI_YES        (int)    1
#define  TRACE_NO        (int)    0
#define  TRACE_YES       (int)    1
#define  TYPE_NONE       (int)    0
#define  TYPE_TRACE      (int)    1
#define  TYPE_OUTG_OVLD  (int)    1
#define  TYPE_OUTG_BUSV  (int)    2
#define  TYPE_OVLD_OUTG  (int)    3
#define  TYPE_BUSV_OUTG  (int)    4
#define  TYPE_OVLD_BOTH  (int)    5
#define  TYPE_BUSV_BOTH  (int)    6
#define  REDUN_KEEP      (int)    0
#define  REDUN_REMOVE    (int)    1
#define	 YES             (int)    1
#define  SKP_SOLN_PROB   (int)    0  /* skip reporting of solution problems */
#define  INC_SOLN_PROB   (int)    1  /* include report of solution problems */
#define  FORM_DATA_FULL  (int)    1
#define  FORM_DATA_BREF  (int)    2  /* BRIEF DATA FORMAT */
#define  FORM_PAGE_WIDE  (int)    3
#define  FORM_PAGE_COMP  (int)    4  /* COMPACT PAGE FORMAT */

#define  HEADER_BREAK_PTI  "................................................................................"
#define  HEADER_COMME_PTI  ".               "
#define  DIS_FAC_FILE_PTI  "DISTRIBUTION FACTOR FILE:"
#define  SUB_DES_FILE_PTI  "SUBSYSTEM DESCRIPTION FILE:"
#define  MON_ELE_FILE_PTI  "MONITORED ELEMENT FILE:"
#define  CON_DES_FILE_PTI  "CONTINGENCY DESCRIPTION FILE:"
#define  CONTIN_EVENT_PTI  "--- C O N T I N G E N C Y   E V E N T S ---"
#define  OPEN_LINE_FR_PTI  "OPEN LINE FROM BUS"
#define  BPA_PF_PROG_VERS  "BPA POWER FLOW PROGRAM VERSION:IPF"
#define  PTI_INTERAC_PSSE  "PTI INTERACTIVE POWER SYSTEM SIMULATOR--PSS/E"
#define  SUM_PROBS_4E_OUT  "Summary of bus and line problems for each outage"
#define  SUM_SYST_SEP_DIV  "Summary of System Separations and Divergences"
#define  SUM_COM_MODE_OUT  "Summary of Common Mode Outages"
#define  OUTG_SIMUL_INPUT  "Outage Simulation Input"
#define  BUS_BUS_VPU_PTI   "X------ BUS -----X  V(PU)"
#define  BUS_VV_A_PU_316   "Bus voltage violation                             Actual    Per unit  Actual / change /(    limits   )"
#define  BUS_VV_A_PU_312   "Bus voltage violation                           actual      per unit actual/(    limits   )"
#define  CONTINGENCY_ANAL  " C O N T I N G E N C Y   A N A L Y S I S "
#define  __O_U_T_A_G_E__   "* * O U T A G E * *"
#define  OVLD_FRO_TO_PTI   "FROM     NAME        TO     NAME    CKT  PRE-CNT  POST-CNT  RATING  PERCENT" 
#define  OVLD_RAT_TY_316   "Overloads                                         Rating    Type    per unit / pst-cont  pre-cont Date in" 
#define  OVLD_RAT_TY_314   "Overloads                                             Rating     Type      per unit / actual"
#define  OVLD_RAT_TY_312   "Overloads                                   Rating     Type      per unit / actual"
#define  OUTG_ID_OWN_314   "--------   Outage  ------------- ID  Owner  Zones  -------  Problem"
#define  OUTG_ID_OWN_312   "--------   Outage  ------------- ID  Owner -------  Problem"

/*********************  stuff to follow powerflow changes **********************/
/* #define  COMO_MOD_NA_312  " %*d%*4c%38[^\n]" */
#define  COMO_MOD_NA_312  " %*d%*3c%38[^\n]"
/*********************  stuff to follow powerflow changes **********************/
#define  TY_CH_OWN_Z_315   "Ty  Ch  Own  Zones  Bus1           Bus2"
#define  TY_CH_OWN_B_312   "Ty  Ch  Own  Bus1           Bus2"
#define  BUSV_FORMAT_PTI   "%*60c%8[^\n]%f%*c%f"
#define  BUSV_FORM_2_PTI   "%*60c%*26c%8[^\n]%f%*c%f"
#define  BUSV_FORM_3_PTI   "%*60c%*26c%*26c%8[^\n]%f%*c%f"
#define  BUSV_FORMAT_316   " %8[^\n]%*c%f%*2c%3[^\n]%*2c%2[^\n]%*24c %f KV %f / %f /( %f, %f)"
#define  BUSV_FORMAT_312   " %8[^\n]%*c%f%*2c%3[^\n]%*2c%2[^\n]%*21c %f KV %f /( %f, %f)" /* extra %f add for alignment */
#define  COMO_FORMAT_314   "%*7c%2[^\n]%*2c%c%*3c%3[^\n]%*2c%2[^\n]%*c%2[^\n]%*2c%*8[^\n]%f %*8[^\n]%f"
#define  COMO_FORMAT_312   "%*6c%2[^\n]%*2c%c%*3c%3[^\n]%1[^\n]%1[^\n]%*8[^\n]%f %*8[^\n]%f"
#define  OVLD_FORMAT_PTI   "%*61c%8[^\n]%f%*1c%*6c%8[^\n]%f%*1c%c %f %f %f %f"
#define  OVLD_FORMAT_316   "%*24c %8[^\n]%f %8[^\n]%f%*1c%c%*1c%d%*2c%3[^\n]%*2c%2[^\n]%*c%2[^\n] %f %*3[^\n]  %c %f / %f %f   %5[^\n]"
#define  OVLD_FORMAT_314   "%*26c%8[^\n]%f %8[^\n]%f%*2c%c%*2c%d%*2c%3[^\n]%*2c%2[^\n]%*c%2[^\n] %f %*3[^\n]  %c %f / %f"
#define  OVLD_FORMAT_312   "%*26c%8[^\n]%f %8[^\n]%f%*2c%c%*2c%d%*2c%3[^\n]%1[^\n]%1[^\n]        %f %*3[^\n]  %c %f / %f"
#define  OUTG_FORMAT_PTI   "%*25c[%8[^\n] %f]%*14c[%8[^\n] %f] CKT %c ---%3[^\n]%*2c%2[^\n]%*1c%2[^\n]"
#define  OUTG_FORMAT_312   " %8[^\n] %f %8[^\n] %f%*2c%c%*2c%3[^\n]%*2c%2[^\n]%*1c%2[^\n]"
#define  SYST_FORMAT_312   " %8[^\n] %f %8[^\n] %f%*2c%c%*4c%3[^\n]%*2c%2[^\n]%*1c%2[^\n]"

#define  MSG_CRTCL     "\n. CRITCL RATING taken from the first non-zero case.\n"
#define  MSG_UNITS     ". Load in AMPS for lines and MVA for transformers.\n"
#define  MSG_FLAGS     ". F Rating flags indicate type of critical rating.\n"
#define  MSG_LWRCS     ". Lowercase flag indicates rating differs from listed value.\n"
#define  MSG_SYMBL     ". T Thermal, N Nominal, B Bottleneck, E Emergency (Loss of Life)\n"
#define  MSG_FALRX     ". FAILED RX   (or RX)   - Failed reactive solution; Solved real power only\n"
#define  MSG_NOSOL     ". NO SOLUTION (or FAIL) - Failed solution\n"
#define  MSG_SEPAR     ". SEPARATION  (or SEPA) - System Separation\n"

#define  SRCHN_4_COMO_SUM  (int) (1<< 0) /* Summary of common mode outages */
#define  SRCHN_4_OUTG_SUM  (int) (1<< 1) /* Sum of bus & line probs 4 ea outg */
#define  SRCHN_4_FAIL_SUM  (int) (1<< 2) /* Sum of system separation & diverg */
#define  READING_COMO_SUM  (int) (1<< 3)
#define  READING_OUTG_SUM  (int) (1<< 4)
#define  READING_FAIL_SUM  (int) (1<< 5)
#define  GET_OVLD          (int) (1<< 6)
#define  GET_BUSV          (int) (1<< 7)
#define  GET_OUTG          (int) (1<< 8)
#define  VALID_DATA        (int) (1<< 9)
#define  DATA_PTI          (int) (1<<10)
#define  SRCHING (int) (SRCHN_4_COMO_SUM | SRCHN_4_OUTG_SUM | SRCHN_4_FAIL_SUM)
#define  READING (int) (READING_COMO_SUM | READING_OUTG_SUM | READING_FAIL_SUM)
#define  HASHSIZE 30
/***************************** end #define ************************************/
/******************************* typedef **************************************/
typedef struct {
  float  actual;  /* post-condition MVA, AMPS, or KV, or absolute (new-ref) */
  float  ratio;   /* PU, per cent, or delta (new-ref/ref) */
  float  min_ref; /* minimum (low limit), or reference */
  float  max_new; /* maximum (high limit), or rating, or new */
  float  pre_con; /* pre-condition MVA, AMPS, or KV */
  char   code;    /* rating code: T, N, E, or B */
  char   date[6]; /* date in MO/YR format */
} ff_soln;

typedef struct traceRecord {
  Link   *pfoList;
  Link   *outgList;
  Link   *comoList;
  Link   *ovldList;
  Link   *busvList;
  Link   *outgMask;
  Link   *comoMask;
  Link   *ovldMask;
  Link   *busvMask;
  Link   *mainTable;
  Link   *rptTable;
  Link   *ownrOutg;
  Link   *zoneOutg;
  Link   *bskvOutg;
  Link   *ownrProb;
  Link   *zoneProb;
  Link   *bskvProb;
  Link   *chkBranchStart;  /* Link to branches in No O/L Log */
  long   query;
  int     sort;       /* sort criteria: i.e. (trace *) or trace->sort */
  int     soln;       /* indicates whether or not to report solution problems */
  int     type;       /* output report type of rpt and aux reports */
  int     redun;
  int	  listSeparations;
  int     nCases;
  float   minLoadPct;
  float   maxDeltaPct;
  float   varCeiling;
  float   varFloor;
  float   max5;
  float   min5;
  float   maxV;
  float   minV;
  cf_Out *trc;
  cf_Out *rpt;
  cf_Out *aux;
  char    outName[FILENAME_MAX];
  char    timeStamp[CF_STRSIZE];
  char    userID[CF_STRSIZE];
  char    IPFversion[CF_STRSIZE];        /* PRG (program) could be IPF or PTI */
} Trace;

struct nlist {  /* table entry */
    struct nlist *next;  /* next entry in chain */
    char         *name;  /* common-mode outage  */
    int           keep;  /* 1 = report, 0 = ignore */
};
/******************************* end typedef **********************************/
/* top FINDOUT  functions - called by main() **********************************/
void     initializeTrace(Trace *trace);
void     processCommandLine(Trace *trace, int argc, char *argv[]);
void     instructions(Trace *trace);
void     promptUser(Trace *trace);
void     openReport(cf_Out *rpt);
void     printTrace(Trace *trace);
void     queryContinue(Trace *trace);
void     collectData(Trace *trace);
void     buildReport(Trace *trace, cf_Out *rpt);
void     printReport(Trace *trace, cf_Out *rpt);
void     swapReport(Trace *trace);
void     finalRemarks(Trace *trace);
/* end FINDOUT  functions - called by main() **********************************/
      
/* top FINDOUT  functions - report functions **********************************/
void     ff_printBlankLines(cf_Out *rpt, int n);
int      ff_printPageFooter(cf_Out *rpt, int m);
void     pageFooter(cf_Out *rpt, Link *nxRow, Link *nxPfo);
void     tableLegend(cf_Out *rpt);
void     printHeader(cf_Out *rpt, Link *curPfo, Trace *trace );
void     printID(cf_Out *rpt, Link *curID, int position);
void     printSoln(cf_Out *rpt, Link *curID, Link *curSoln, int doN);
void     printOvldHeader(cf_Out *rpt, Link *curPfo);
void     printOtBrHeader(cf_Out *rpt, Link *curPfo);
void     printOtBsHeader(cf_Out *rpt, Link *curPfo);
void     printBusvHeader(cf_Out *rpt, Link *curPfo);
void     printTableCases(cf_Out *rpt, Link *curPfo, char *l, char *r, char *d);
void     printTableHeader(cf_Out *rpt, Link *curPfo, char *l, char *r, char *d);
void     printOutg(FILE *fp, Link *outgLink, int form);
void     printOvld(FILE *fp, Link *ovldLink, int form);

void     chkBranch(Trace *trace, Link *curID);             /* No O/L Log support */
void     chkBranchLog(Trace *trace, Link *curPfo);         /* No O/L Log support */

void     printBusv(FILE *fp, Link *busvLink, int form);
void     printLoad(cf_Out *rpt, Link *loadLink, int doN);
void     printVolt(cf_Out *rpt, Link *voltLink, int doN);
void     printProb(cf_Out *rpt, Link *probLink, int doN);
void     printDiff(cf_Out *rpt, Link *solnLink, int doN);
/* end FINDOUT  functions - report functions **********************************/

/* top FINDOUT  functions - support functions *********************************/
int      screenProb(Link *problem, Trace trace);
int      buildMainTable(FILE *pfo_File, Trace *trace);
Link    *getOutgSet(FILE *pfo_File, int *state, Link *problem, Trace *trace);
int      getCoMoRslt(char *s, char *format, Trace trace);
int      getOvldLoad(char *s, char *format, Link *problem);
int      getBusvVolt(char *s, char *format, Link *problem);
Link    *getSystProb(char *s, char *format, Link *problem);
Link    *getBrchOutg(char *s, char *format);
Link    *getCoMoOutg(char *s);
int      identifyInput(char *s);
void     installResult(char *s);
int      ff_srtByInput(Link *newRow, Link *xstRow, void *sort);
int      srtByInput(Link *newRow, Link *xstRow, Trace trace);
int      getIndex(Link *newRow, Link *xstRow, Link *list);
int      validVariance(Trace trace, Link *solnLink);
int      validLoad(Trace trace, Link *solnPtr);
int      validVolt(Trace trace, Link *solnPtr);
int      validSoln(Link *solnPtr, Trace *trace);
int      validInput(Link *listPtr, cf_Branch *data);
void     swapRowIdLinks(Link *rowPtr);
int      validMask(Link *maskList, cf_Name *r);
int      ff_srtBus(pf_AC_bus *b1, pf_AC_bus *b2, int sort); /* sort bus */
int      ff_srtSoln(Link *newSoln, Link *xstSoln, int sort);
int      ff_cmpType(Link *newId, Link *xstId);
int      ff_cmpName(cf_Name *b1, cf_Name *b2);
int      ff_cmpID(Link *newLink, Link *xstLink);
int      ff_srtID(Link *newLink, Link *xstLink, int sort);
int      ff_cmpRow(Link *newRow, Link *xstRow, int sort);
/* end FINDOUT  functions - support functions *********************************/

/* top LINEFLOW, FINDOUT, MIMIC - similar functions ***************************/
void     ff_stream2List(FILE *readMe, Trace *trace, Link **expList);
void     ff_report(char *s, Trace *trace);
void     ff_limits(char *s, Trace *trace);
void     ff_expList(Trace *trace, Link *dataList, Link **expList);
void     ff_appSoln2Row(Link *curRow, Link *solnLink, int nCases);
int      ff_srtBranch(cf_Branch *b1, cf_Branch *b2, int sort);
int      ff_wildBranch(pf_branch *b1, pf_branch *b2);
/* end LINEFLOW, FINDOUT, MIMIC - similar functions ***************************/

/* top CF_UTIL.H candidate functions ******************************************/
void date(char *date);
/* end CF_UTIL.H candidate functions ******************************************/

/* top FINDOUT  functions - hash table functions ******************************/
unsigned hash(char *s);
struct nlist *install(char *name, int keep);
struct nlist *lookup(char *s);
/* end FINDOUT  functions - hash table functions ******************************/

/******************************** global variable candidates ******************/
static struct nlist *hashtab[HASHSIZE]; /* pointer table or cm outages */
/**************************** end global variable candidates ******************/

cf_Style FF_mskStyl = { "",   CF_TAG_W_LIST,  1 };

int main(int argc, char *argv[])
/***************************************************************************\
*  FINDOUT PROGRAM START
*
\***************************************************************************/
{
  Trace trace;

  time(&CF_time0);

  initializeTrace(&trace);

  processCommandLine(&trace, argc, argv);

  instructions(&trace);

  promptUser(&trace);

  printTrace(&trace);

  queryContinue(&trace);

  collectData(&trace);

  trace.chkBranchStart = cf_dupList (trace.ovldList);       /* Link to No O/L Log */

  buildReport(&trace, trace.rpt);

  printReport(&trace, trace.rpt);

  swapReport(&trace);

  printReport(&trace, trace.aux);

  finalRemarks(&trace);

  time(&CF_time1);

  cf_logUse("FindOut", trace.IPFversion, trace.userID);

  return 0;
}

void initializeTrace(Trace *trace)
/***************************************************************************\
* special function to do set up/init for FINDOUT 
*
\***************************************************************************/
{
  memset(trace, '\0', sizeof(Trace));
  trace->query      =  ( QUERY_PFO  | QUERY_DATA | QUERY_DIFF |
                         QUERY_SOLN | QUERY_TYPE | QUERY_NAME |
                         QUERY_LOAD | QUERY_VOLT | QUERY_SORT |
                         QUERY_OWNR | QUERY_ZONE | QUERY_BSKV |
                         QUERY_OW_P | QUERY_ZN_P | QUERY_KV_P |
                         QUERY_FORM | QUERY_WDTH | QUERY__TRC |
                         QUERY_CONT | QUERY_OUTG | QUERY_COMO |
                         QUERY_OVLD | QUERY_BUSV | QUERY_DLTA |
                         QUERY_REDU | QUERY_VARI | QUERY_SPAN |
			 QUERY_SSEP );

  trace->varCeiling =  1;
  trace->varFloor   = -1;
  trace->minLoadPct = DFLT_MIN_LOAD_PCT;
  trace->maxDeltaPct= DFLT_MAX_DELTA_PCT;
  trace->max5       = DFLT_MAX_500_PU;
  trace->min5       = DFLT_MIN_500_PU;
  trace->maxV       = DFLT_MAX_BUS_PU;
  trace->minV       = DFLT_MIN_BUS_PU;
  trace->sort       = SORT_BY_ALPHA;
  trace->type       = TYPE_OUTG_OVLD;
  trace->redun      = REDUN_KEEP;
  trace->listSeparations = 0;
  trace->nCases     = 0;
  trace->IPFversion[0] = '\0';
  cuserid(trace->userID);
  cf_time(trace->timeStamp, CF_STRSIZE, CF_TIMESTAMP);
  strcpy(CF_logSpec, LOG_SPEC);
  strcpy(trace->outName, OUT_NAME);
  trace->trc = cf_initOut("", trace->timeStamp, TRACE_YES, FORM_DATA_FULL);
  trace->rpt = cf_initOut("", trace->timeStamp, TYPE_NONE, FORM_DATA_FULL);
  trace->aux = cf_initOut("", trace->timeStamp, TYPE_NONE, FORM_DATA_FULL);
}


void processCommandLine(Trace *trace, int argc, char *argv[])
{
  Link *list, *topLink;
  int   i;
  topLink = NULL;
  if (argc <= 1) return;
  for (i = argc; --i > 0; cf_appList(&topLink, list)) { 
    list = cf_text2Link(argv[i]);
  }
  ff_expList(trace, topLink, &trace->pfoList);
  return;
}

void  instructions(Trace *trace)
/****************************************************************************\
*
\****************************************************************************/
{
  if (trace->query==0) return;/* skip if there is not going to be any queries */
  printf("\n*=================================================*");
  printf("\n|                  FINDOUT                        |");
  printf("\n|      Version %3d        updated %8s        |", 
	VERSN_NUM, VERSN_DATE );
  printf("\n*=================================================*\n");
  printf("\n - Defaults in brackets [].  Press Ctrl-Y & type STOP to abort.");
  printf("\n - Use .trc, .dat, or .lis or append / to designate data files.");
  printf("\n - Powerflow version %d or later recommended.", FF_IPF_VERSION);
  printf("\n");
}

void promptUser(Trace *trace)
/****************************************************************************\
*
\****************************************************************************/
{
  char  query[CF_INBUFSIZE];
  Link *list;

  printf("%s", DOT_LINE);

  if (trace->query & QUERY_PFO) {
    printf("\n\n/PFO, /TRACE, or /INCLUDE");
    cf_nprompt("\n > Enter list of .PFO files (and/or data files): ", "", CF_INBUFSIZE, query);
    list = cf_text2List(query);

    ff_expList(trace, list, &trace->pfoList);
  }

  if (trace->pfoList==NULL) {
    printf("\n\n *** Warning ***, No *.PFO files have been specified!");
    printf("\n\n/PFO, /TRACE, or /INCLUDE");
    cf_nprompt("\n > Enter list of .PFO files (and/or data files): ", "", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    ff_expList(trace, list, &trace->pfoList);
  }

  cf_exit(trace->pfoList==NULL, "No data or *.PFO files!  Quitting!\n");

  if (trace->query == QUERY_CONT) trace->query &= ~(QUERY_CONT);

  if (cf_cntLink(trace->pfoList)==1) {
    trace->query &= ~(QUERY_WDTH);             /* only one *.pfo file present */
  }

  if (trace->query &
        (QUERY_NAME|QUERY_TYPE|QUERY_SORT|QUERY_FORM|QUERY_WDTH|QUERY_REDU)) {
    printf("\n\n/REPORT");
  }

  if (trace->query & QUERY_NAME) {
    cf_sprompt("\n > Enter output files default name, NAME = [%s]: ", trace->outName, trace->outName);
    printf("\n");
  }

  if (trace->query & QUERY_TYPE) {
    printf("\n Specify the type of output report (by number)");
    printf("\n   1. OUTAGEs  causing  OVERLOADs    2. OUTAGEs  causing  BUS Violations");
    printf("\n   3. OVERLOADs caused by OUTAGEs    4. BUS Violations caused by OUTAGES");
    printf("\n   5. OVERLOADs BOTH WAYS (1 & 3)    6. BUS Violations BOTH WAYS (2 & 4)");
    cf_iprompt("\n > Enter report choice, TYPE =[OUTAGE-OVERLOAD]: ", trace->type, &trace->type);
    printf("\n");
  }

  switch (trace->type)
  {
    case 1:
      trace->rpt->type = TYPE_OUTG_OVLD;
      trace->aux->type = 0;
    break;

    case 2:
      trace->rpt->type = TYPE_OUTG_BUSV;
      trace->aux->type = 0;
    break;

    case 3:
      trace->rpt->type = TYPE_OVLD_OUTG;
      trace->aux->type = 0;
    break;

    case 4:
      trace->rpt->type = TYPE_BUSV_OUTG;
      trace->aux->type = 0;
    break;

    case 5:
      trace->rpt->type = TYPE_OUTG_OVLD;
      trace->aux->type = TYPE_OVLD_OUTG;
    break;

    case 6:
      trace->rpt->type = TYPE_OUTG_BUSV;
      trace->aux->type = TYPE_BUSV_OUTG;
    break;
  }

  if (cf_cntLink(trace->pfoList)==1) {         /* only one *.pfo file present */
    trace->query &= ~(QUERY_VARI);
    trace->query &= ~(QUERY_SPAN);
    trace->trc->diff = VARI_NO;
  }

  if (trace->query & QUERY_VARI) {
    printf("\n There is more than one .PFO file\n");
    printf("  Variance = No   List ALL overloads.\n");
    printf("  Variance = Yes  List overload if not initially overloaded in 1st .PFO file.\n");
    trace->trc->diff = cf_yprompt("\n > Do a variance report?       VARIANCE = [YES]: ", 'Y');
    printf("\n");
  }

  if (trace->trc->diff==VARI_NO) trace->query &= ~(QUERY_SPAN);
  trace->rpt->diff = trace->trc->diff;
  trace->aux->diff = trace->trc->diff;

  if (trace->query & QUERY_REDU) {
    trace->redun = cf_yprompt("\n > Redundant outages:   REMOVE_REDUNDANT = [NO]: ", 'N');
    printf("\n");
  }

  if (trace->query & QUERY_SSEP) {
    trace->listSeparations = cf_yprompt("\n > List system separations:   LIST_SEPARATIONS = [NO]: ", 'N');
  }

  if (trace->query & QUERY_SORT) {
    printf("\n Specify sort priority (by number)");
    printf("\n   1. ALPHA-base_kv   3. OWNER-alpha-base_kv    5. INPUT order");
    printf("\n   2. BASE_KV-alpha   4.  ZONE-alpha-base_kv    6. SOLUTION");
    cf_iprompt("\n > Enter sort choice,            SORT = [ALPHA]: ", trace->sort, &trace->sort);
    printf("\n");
  }

  if (trace->sort==SORT_BY_SOLN && trace->trc->diff==VARI_YES) {
    trace->sort = SORT_BY_DIFF;
  }

  if (trace->query & QUERY_FORM) {
    printf("\n Specify output report format (by number)");
    printf("\n   1. DATA-FULL (Excel)  2. DATA-BRIEF (Excel)  3. PAGE-WIDE  4. PAGE-COMPACT");
    cf_iprompt("\n > Enter format choice,    FORMAT = [DATA-FULL]: ", trace->trc->form, &trace->trc->form);
    printf("\n");
  }

  trace->rpt->form = trace->trc->form;
  trace->aux->form = trace->trc->form;
  
  if (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG) {
    if (trace->query & QUERY_WDTH) {   /* get default width */
        if      (trace->trc->form==FORM_DATA_BREF) trace->trc->wide = 9;
        else if (trace->trc->form==FORM_PAGE_COMP) trace->trc->wide = 4;
        else                                       trace->trc->wide = 2;
    }
  }
  else if (trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG) {
    if (trace->query & QUERY_WDTH) {                     /* get default width */
        if      (trace->trc->form==FORM_DATA_BREF) trace->trc->wide = 12;
        else if (trace->trc->form==FORM_PAGE_COMP) trace->trc->wide = 4;
        else                                       trace->trc->wide = 3;
    }
  }

  if (trace->query & QUERY_WDTH) {
    printf("\n");
    printf("\n Specify the maximum number of cases per table (%d fit in 132 columns).", trace->trc->wide);
    cf_iprompt("\n > Enter width in cases,  CASES_PER_TABLE = [%d]: ", trace->trc->wide, &trace->trc->wide);
  }

  trace->rpt->wide = trace->trc->wide;
  trace->aux->wide = trace->trc->wide;
  if (trace->query & QUERY_DATA) {
    printf("\n\n/INCLUDE");
    cf_nprompt("\n > Enter list of files with branch or bus lists: ", "", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    ff_expList(trace, list, NULL);
  }

  if (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG)
    trace->query &= ~(QUERY_BUSV | QUERY_VOLT);

  if (trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG)
    trace->query &= ~(QUERY_OVLD | QUERY_LOAD);

  if (trace->query & QUERY_OUTG) {
    printf("\n\n/OUTAGE"); 
    printf("\n > Enter outaged branches list       :"); 
    printf("\n   > Tycown< BUS1 ><V1> < BUS2 ><V2>cs");
    do {
        cf_nprompt("\n   > ", "", CF_INBUFSIZE, query);
        if (strchr(query, FF_WILD)==NULL) {
            list = cf_id2Link(query, 'L');
            cf_appList(&trace->outgList, list);
        }
        else {
            list = cf_text2Link(query);
            cf_appList(&trace->outgMask, list);
        }
        if ( list!=NULL && (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OUTG_BUSV) )
            trace->query &= ~(QUERY_ZONE | QUERY_OWNR | QUERY_BSKV);
    } while (!cf_isblank(query));
  }

  if (trace->query & QUERY_COMO) {
    printf("\n/COMMON_MODE"); 
    printf("\n > Enter common-mode outages list    :");
    do {
        cf_nprompt("\n   > ", "", CF_INBUFSIZE, query);
        if (strchr(query, FF_WILD)==NULL) {
            list = getCoMoOutg(query);
            cf_appList(&trace->comoList, list);
        }
        else {
            list = cf_text2Link(query);
            cf_appList(&trace->comoMask, list);
        }
        if ( list!=NULL && (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OUTG_BUSV) )
            trace->query&= ~(QUERY_ZONE | QUERY_OWNR | QUERY_BSKV);
    } while (!cf_isblank(query));
    if (list!=NULL && (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OUTG_BUSV))
        trace->query &= ~(QUERY_ZONE | QUERY_OWNR | QUERY_BSKV);
  }

  if (trace->query & (QUERY_OWNR | QUERY_ZONE | QUERY_BSKV)) {
    printf("\n\n/OUTG_OWNER, /OUTG_ZONE, /OUTG_BASE_KV");
    printf("\n Specify selection of branch OUTAGEs and COMMON_MODE outages");
  }

  if (trace->query & QUERY_OWNR) {
    cf_nprompt("\n > Enter owners of interest..[all owners='%s']: ", "***", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    cf_appList(&trace->ownrOutg, list);
  }

  if (trace->query & QUERY_ZONE) {
    cf_nprompt("\n > Enter zones of interest.....[all zones='%s']: ", "**", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    cf_appList(&trace->zoneOutg, list);
  }

  if (trace->query & QUERY_BSKV) {
    cf_nprompt("\n > Enter base_kv of interest.[all base kV='%s']: ", ">0", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    cf_appList(&trace->bskvOutg, list);
  }

  if (trace->query & QUERY_OVLD) {
    printf("\n\n/OVERLOAD");
    printf("\n > Enter overloaded branches list    :");
    printf("\n   > Tycown< BUS1 ><V1> < BUS2 ><V2>cs");
    do {
        cf_nprompt("\n   > ", "", CF_INBUFSIZE, query);
        if (strchr(query, FF_WILD)==NULL) {
            list = cf_id2Link(query, 'L');
            cf_appList(&trace->ovldList, list);
        }
        else {
            list = cf_text2Link(query);
            cf_appList(&trace->ovldMask, list);
        }
        if ( list!=NULL && (trace->rpt->type==TYPE_OVLD_OUTG || trace->rpt->type==TYPE_BUSV_OUTG) )
            trace->query&= ~(QUERY_ZN_P | QUERY_OW_P | QUERY_KV_P);
    } while (!cf_isblank(query));
    if (list!=NULL && (trace->rpt->type==TYPE_OVLD_OUTG || trace->rpt->type==TYPE_BUSV_OUTG))
        trace->query &= ~(QUERY_ZN_P | QUERY_OW_P | QUERY_KV_P);
  }

  if (trace->query & QUERY_BUSV) {
    printf("\n\n/BUS");
    printf("\n > Enter busses list  :");
    printf("\n   > Tycown< NAME ><KV>");
    do {
        cf_nprompt("\n   > ", "", CF_INBUFSIZE, query);
        if (strchr(query, FF_WILD)==NULL) {
            list = cf_id2Link(query, 'I');
            cf_appList(&trace->busvList, list);
        }
        else {
            list = cf_text2Link(query);
            cf_appList(&trace->busvMask, list);
        }
        if ( list!=NULL && (trace->rpt->type==TYPE_OVLD_OUTG || trace->rpt->type==TYPE_BUSV_OUTG) )
            trace->query &= ~(QUERY_ZN_P | QUERY_OW_P | QUERY_KV_P);
    } while (!cf_isblank(query));
    if (list!=NULL && (trace->rpt->type==TYPE_OVLD_OUTG || trace->rpt->type==TYPE_BUSV_OUTG))
        trace->query &= ~(QUERY_ZN_P | QUERY_OW_P | QUERY_KV_P);
  }

  if (trace->query & (QUERY_OW_P | QUERY_ZN_P | QUERY_KV_P)) {
    printf("\n\n/PROB_OWNER, /PROB_ZONE, /PROB_BASE_KV");
    if (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG)
        printf("\n Specify selection of OVERLOADed branches");
    if (trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG)
        printf("\n Specify selection of over- or under-voltage BUSses");
  }

  if (trace->query & QUERY_OW_P) {
    cf_nprompt("\n > Enter owners of interest..[all owners='%s']: ", "***", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    cf_appList(&trace->ownrProb, list);
  }

  if (trace->query & QUERY_ZN_P) {
    cf_nprompt("\n > Enter zones of interest.....[all zones='%s']: ", "**", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    cf_appList(&trace->zoneProb, list);
  }

  if (trace->query & QUERY_KV_P) {
    cf_nprompt("\n > Enter base_kv of interest.[all base kV='%s']: ", ">0", CF_INBUFSIZE, query);
    list = cf_text2List(query);
    cf_appList(&trace->bskvProb, list);
  }

  if (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG) {
    trace->query &= ~(QUERY_VOLT);
  }

  if (trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG) {
    trace->query &= ~(QUERY_LOAD);
  }

  if (trace->query & (QUERY_LOAD | QUERY_VOLT | QUERY_SPAN | QUERY_SOLN)) {
    printf("\n\n/LIMITS");
  }

  if (trace->query & QUERY_SPAN) {
    if (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG) {
        cf_fprompt("\n > Enter variance ceiling (%%),VARI_CEILING=[%2.0f]: ", 1.0, &trace->varCeiling);
        cf_fprompt("\n > Enter variance floor (%%),  VARI_FLOOR  =[%2.0f]: ",-1.0, &trace->varFloor);
    }
    if (trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG) {
        cf_fprompt("\n > Enter variance ceiling PU, VARI_CEILING =[%3.3f]: ", .01, &trace->varCeiling);
        cf_fprompt("\n > Enter variance floor PU,   VARI_FLOOR   =[%3.3f]: ",-.01, &trace->varFloor);
    }
  }

  if (trace->query & QUERY_LOAD) {
    cf_fprompt("\n > Enter min loading in %%, MIN_LOAD_PCT = [%2.0f%%]: ", trace->minLoadPct, &trace->minLoadPct);
  }
  if (trace->query & QUERY_VOLT) {
    cf_fprompt("\n > Max 500kV-bus voltage PU, MAX_500_PU=[%5.3f]: ", trace->max5, &trace->max5);
    cf_fprompt("\n > Min 500kV-bus voltage PU, MIN_500_PU=[%5.3f]: ", trace->min5, &trace->min5);
    cf_fprompt("\n > Max sub-500kV voltage PU, MAX_BUS_PU=[%5.3f]: ", trace->maxV, &trace->maxV);
    cf_fprompt("\n > Min sub-500kV voltage PU, MIN_BUS_PU=[%5.3f]: ", trace->minV, &trace->minV);
  }

  if (trace->query & QUERY_SOLN) {
    printf("\n");
    trace->soln = cf_yprompt("\n > Include solution problems? INC_SOLN_PROB=[%c]: ", 'Y');
  }

  printf("\n");
  cf_strsuf(trace->rpt->spec, trace->outName, '.', ".rpt");
  cf_strsuf(trace->aux->spec, trace->outName, '.', ".aux");
  cf_strsuf(trace->trc->spec, trace->outName, '.', ".trc");
  printf("%s\n", DOT_LINE);
  return;
}

void openReport(cf_Out *rpt)
{
  rpt->file = cf_openFile(rpt->spec, "w");
  cf_exit(rpt->file==NULL, "Quitting!\n");
}

void printTrace(Trace *trace)
/****************************************************************************\
*
\****************************************************************************/
{
  FILE *fp;

  if (trace->trc->type==TYPE_NONE) return;

  openReport(trace->trc);

  fp = trace->trc->file;
  if (fp == NULL) fp = stderr;
  fprintf(fp, ". %s %s %s %s\n", trace->trc->spec, trace->trc->time,
    trace->userID, trace->IPFversion);

  cf_printList(fp, trace->pfoList, CF_oneStyl, "/PFO\n");
  fprintf(fp, "/REPORT\n");
  fprintf(fp, "  NAME = %s\n", trace->outName);

  if (trace->type     ==TYPE_OUTG_OVLD) fprintf(fp,"  TYPE = OUTAGE-OVERLOAD\n");
  if (trace->type     ==TYPE_OUTG_BUSV) fprintf(fp,"  TYPE = OUTAGE-BUS_V\n");
  if (trace->type     ==TYPE_OVLD_OUTG) fprintf(fp,"  TYPE = OVERLOAD-OUTAGE\n");
  if (trace->type     ==TYPE_BUSV_OUTG) fprintf(fp,"  TYPE = BUS_V-OUTAGE\n");
  if (trace->type     ==TYPE_OVLD_BOTH) fprintf(fp,"  TYPE = OVERLOAD-BOTH-WAYS\n");
  if (trace->type     ==TYPE_BUSV_BOTH) fprintf(fp,"  TYPE = BUS_V-BOTH-WAYS\n");
  if (trace->sort     ==SORT_BY_ALPHA ) fprintf(fp,"  SORT = ALPHA\n");
  if (trace->sort     ==SORT_BY_BASEKV) fprintf(fp,"  SORT = BASE_KV\n");
  if (trace->sort     ==SORT_BY_OWNER ) fprintf(fp,"  SORT = OWNER\n");
  if (trace->sort     ==SORT_BY_ZONE  ) fprintf(fp,"  SORT = ZONE\n");
  if (trace->sort     ==SORT_BY_INPUT ) fprintf(fp,"  SORT = INPUT\n");
  if (trace->sort     ==SORT_BY_SOLN  ) fprintf(fp,"  SORT = SOLUTION\n");
  if (trace->sort     ==SORT_BY_DIFF  ) fprintf(fp,"  SORT = SOLUTION\n");
  if (trace->trc->form==FORM_DATA_FULL) fprintf(fp,"  FORMAT = DATA_FULL\n");
  if (trace->trc->form==FORM_DATA_BREF) fprintf(fp,"  FORMAT = DATA_BRIEF\n");
  if (trace->trc->form==FORM_PAGE_WIDE) fprintf(fp,"  FORMAT = PAGE_WIDE\n");
  if (trace->trc->form==FORM_PAGE_COMP) fprintf(fp,"  FORMAT = PAGE_COMPACT\n");
  if (trace->redun    ==REDUN_REMOVE  ) fprintf(fp,"  REMOVE_REDUNDANT = YES\n");
  if (trace->redun    ==REDUN_KEEP    ) fprintf(fp,"  REMOVE_REDUNDANT = NO\n");
  if (trace->listSeparations!=YES     ) fprintf(fp,"  LIST_SEPARATIONS = NO\n");
  if (trace->listSeparations==YES     ) fprintf(fp,"  LIST_SEPARATIONS = YES\n");
  if (trace->trc->type==TYPE_TRACE    ) fprintf(fp,"  TRACE = YES\n");
  if (trace->trc->type!=TYPE_TRACE    ) fprintf(fp,"  TRACE = NO\n");
  if (trace->trc->diff==VARI_YES      ) fprintf(fp,"  VARIANCE = YES\n");
  if (trace->trc->diff==VARI_NO       ) fprintf(fp,"  VARIANCE = NO\n");

  fprintf(fp, "  CASES_PER_TABLE = %d\n", trace->trc->wide);
  fprintf(fp, "/LIMITS\n");

  if (trace->trc->diff==VARI_YES) {
    fprintf(fp, "  VARIANCE_CEILING = %6.4f\n", trace->varCeiling);
    fprintf(fp, "  VARIANCE_FLOOR   = %6.4f\n", trace->varFloor);
  }

  if (trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG) {
    fprintf(fp, "  MIN_LOAD_PCT = %6.4f\n", trace->minLoadPct);
  }

  if (trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG) {
    fprintf(fp, "  MAX_500_PU = %6.4f\n", trace->max5);
    fprintf(fp, "  MIN_500_PU = %6.4f\n", trace->min5);
    fprintf(fp, "  MAX_BUS_PU = %6.4f\n", trace->maxV);
    fprintf(fp, "  MIN_BUS_PU = %6.4f\n", trace->minV);
  }

  if (trace->soln == INC_SOLN_PROB  ) fprintf(fp,"  INC_SOLN_PROB = YES\n");
  if (trace->soln == SKP_SOLN_PROB  ) fprintf(fp,"  INC_SOLN_PROB = NO\n");

  cf_printList(fp, trace->ownrOutg, CF_dznStyl, "/OUTG_OWNER\n");
  cf_printList(fp, trace->zoneOutg, CF_dznStyl, "/OUTG_ZONE\n");
  cf_printList(fp, trace->bskvOutg, CF_sixStyl, "/OUTG_BASE_KV\n");
  cf_printList(fp, trace->ownrProb, CF_dznStyl, "/PROB_OWNER\n");
  cf_printList(fp, trace->zoneProb, CF_dznStyl, "/PROB_ZONE\n");
  cf_printList(fp, trace->bskvProb, CF_sixStyl, "/PROB_BASE_KV\n");
  cf_printList(fp, trace->outgList, FF_mskStyl, "/OUTAGE\n.ycown< BUS1 ><V1> < BUS2 ><V2>cs\n");
  cf_printList(fp, trace->outgMask, FF_mskStyl, "/OUTAGE\n.ycown< BUS1 ><V1> < BUS2 ><V2>cs\n");
  cf_printList(fp, trace->comoList, FF_mskStyl, "/COMMON_MODE\n");
  cf_printList(fp, trace->comoMask, FF_mskStyl, "/COMMON_MODE\n");
  cf_printList(fp, trace->ovldList, FF_mskStyl, "/OVERLOAD\n.ycown< BUS1 ><V1> < BUS2 ><V2>cs\n");
  cf_printList(fp, trace->ovldMask, FF_mskStyl, "/OVERLOAD\n.ycown< BUS1 ><V1> < BUS2 ><V2>cs\n");
  cf_printList(fp, trace->busvList, FF_mskStyl, "/BUS\n.ycown< BUS  ><KV>zn\n");
  cf_printList(fp, trace->busvMask, FF_mskStyl, "/BUS\n.ycown< BUS  ><KV>zn\n");
}

void queryContinue(Trace *trace)
/****************************************************************************/
{
  int  yes;

  if (trace->trc->type==TRACE_YES)
    printf("\nTrace written to %s", trace->trc->spec);

  if (CF_logFile != NULL)
    printf("\nError Summary written to %s", CF_logSpec);

  if ( trace->query & QUERY_CONT) {
    yes= cf_yprompt("\n > Do you want to continue this run?        [%c]: ",'Y');
    cf_exit(!yes, "Quitting!");
  }
}

void collectData(Trace *trace)
/****************************************************************************\
*
\****************************************************************************/
{
  FILE *pfo_File;
  Link *curPfo;

  trace->mainTable = NULL;
  curPfo=trace->pfoList;

  while (curPfo!=NULL) {
    if ( (pfo_File = cf_openFile(curPfo->data, "r")) == NULL ) {
        cf_logErr(" Cannot open this *.pfo file: %s\n", curPfo->data);
        curPfo = cf_delLink(&trace->pfoList, curPfo);
        continue;
    }

    fprintf(stdout, " Searching %s\n", curPfo->data);

    if (buildMainTable(pfo_File, trace)==0) {
        printf("   No data in this .pfo file.\n");
        curPfo = cf_delLink(&trace->pfoList, curPfo);
        continue;
    }
    fclose(pfo_File);
    trace->nCases++;
    curPfo=curPfo->next;
  }
}

void  finalRemarks(Trace *trace)
{
  printf("\n");
  printf("\nMemory allocation (bytes): Cur:%ld Max:%ld Alloc:%ld Freed:%ld\n",
    CF_memCurAlloc, CF_memMaxAlloc, CF_memTotAlloc, CF_memTotFreed);
  if (trace->rpt->type!=TYPE_NONE)
    printf("\nOutput report written to %s", trace->rpt->spec);
  if (trace->aux->type!=TYPE_NONE)
    printf("\nOutput report written to %s", trace->aux->spec);
  if (trace->trc->type!=TYPE_NONE)
    printf("\nTrace  report written to %s", trace->trc->spec);
  if (CF_logFile != NULL) printf("\nError  report written to %s", CF_logSpec);
  printf("\n");
  return;
}

void tableLegend(cf_Out *rpt)
{
  fprintf(rpt->file,"%s", MSG_CRTCL);
  fprintf(rpt->file,"%s", MSG_UNITS);
  fprintf(rpt->file,"%s", MSG_FLAGS);
  fprintf(rpt->file,"%s", MSG_LWRCS);
  fprintf(rpt->file,"%s", MSG_SYMBL);
  fprintf(rpt->file,"%s", MSG_FALRX);
  fprintf(rpt->file,"%s", MSG_NOSOL);
  fprintf(rpt->file,"%s", MSG_SEPAR);
  rpt->line += 9;
}

int buildMainTable(FILE *pfo_File, Trace *trace)
/***************************************************************************\
*
\***************************************************************************/
{
  int      state;
  Link    *curRow, *problem, *newRow, *newSoln, *newProb;
  state = (SRCHN_4_COMO_SUM | SRCHN_4_OUTG_SUM | SRCHN_4_FAIL_SUM);
  curRow = trace->mainTable;
/*  system("show time"); */

  problem = NULL;
  trace->IPFversion[0] = '\0';

  while ( (problem=getOutgSet(pfo_File, &state, problem, trace)) != NULL ) {
    if (screenProb(problem, *trace)==0) continue;
    newProb = cf_dupList(problem);
    newSoln = newProb->next->next;
    newProb->next->next = NULL;
    newRow = cf_link2row(newProb);
    cf_insLink(&trace->mainTable, &curRow, newRow, (long) SORT_BY_ALPHA,
        CF_INS_FREE, (int (*)(Link*, Link*, int))(ff_cmpRow));
    ff_appSoln2Row(curRow, newSoln, trace->nCases+1);/* add 1 to skip prob id */
  }

/*  system("show time"); */
  printf(" IPF Executable Used: version = %3.3s\n", trace->IPFversion);

  return (state & VALID_DATA) ? 1 : 0;
}

int screenProb(Link *problem, Trace trace)
/****************************************************************************\
* return 0 if problem is to be skipped over, else non-zero
*
\****************************************************************************/
{
  cf_Branch *outg, *ovld;
  cf_Bus    *busv;
  int        type, vo = 0, vp = 0;
  type = trace.type;
  outg = (cf_Branch *) problem->data;
  if ( outg->type[0]=='c' ) {                        /* common mode outages */
    if ( validInput(trace.comoList, outg)==1 ) vo = 1;
    if ( validMask(trace.comoMask, (cf_Name *) outg)==1 ) vo = 1;
    if ( trace.zoneOutg!=NULL ||
         trace.ownrOutg!=NULL ||
         trace.bskvOutg!=NULL )
            vo = 1;
  }
  else if ( outg->type[0]!='c' ) {                        /* branch outages */
    cf_branch_l2h(outg);
    if ( validInput(trace.outgList, outg)==1 ) vo = 1;
    if ( validMask(trace.outgMask, (cf_Name *) outg)==1 ) vo = 1;
    if ( cf_validZone(trace.zoneOutg,outg->bus1_zone,outg->bus2_zone)==1 &&
         cf_validOwner(trace.ownrOutg,outg->owner)==1 &&
         cf_validBasekv(trace.bskvOutg,outg->bus1_kv,outg->bus2_kv)==1 )
            vo = 1;
  }
  if (vo==0) return 0;

  if (type==TYPE_OUTG_OVLD || type==TYPE_OVLD_OUTG || type==TYPE_OVLD_BOTH) {
    ovld = (cf_Branch *) problem->next->data;
    if ( ovld->type[0]=='s' ) return trace.soln;     /*include system problems*/
    if ( ovld->type[0]=='B' ) return 0;              /*skip voltage violations*/
    cf_branch_l2h(ovld);
    if ( validInput(trace.ovldList, ovld)==1 ) vp = 1;
    if ( validMask(trace.ovldMask, (cf_Name *) ovld)==1 ) vp = 1;
    if ( cf_validZone(trace.zoneProb,ovld->bus1_zone,ovld->bus2_zone)==1  &&
         cf_validOwner(trace.ownrProb, ovld->owner)==1 && 
         cf_validBasekv(trace.bskvProb, ovld->bus1_kv, ovld->bus2_kv)==1 )
            vp = 1;
  }
  if (type==TYPE_OUTG_BUSV || type==TYPE_BUSV_OUTG || type==TYPE_BUSV_BOTH
        && problem->next !=0 ) {
    busv = (cf_Bus *) problem->next->data;
    if ( busv->type[0]=='s' ) return trace.soln; /*include system problems*/
    if ( busv->type[0]!='B' ) return 0;                     /* skip overloads */
    if ( validInput(trace.busvList, (cf_Branch *) busv)==1 ) vp = 1;
    if ( validMask(trace.busvMask, (cf_Name *) busv)==1 )  vp = 1;
    if ( cf_validZone(trace.zoneProb, busv->zone, busv->zone)==1 &&
         cf_validOwner(trace.ownrProb, busv->owner)==1 && 
         cf_validBasekv(trace.bskvProb, busv->kv, busv->kv)==1 )
            vp = 1;
  }
  return vp;
}

Link *getOutgSet(FILE *pfo_File, int *state, Link *problem, Trace *trace)
/***************************************************************************\
* searching the summary of bus and line problems for each outage 
*
* Called by: buildMainTable
\***************************************************************************/
{
  long  id, pos;
  char  s[CF_INPFOSIZE+1], name[39], *end;
  static char format[132];
  struct nlist *np;

  while (1) {
    pos = ftell(pfo_File);      /* needed to get 3 buses per line in PTI data */
    if (fgets(s, CF_INPFOSIZE, pfo_File)==NULL) break;

    if (s[0]=='0' || s[0]=='1') s[0]=' ';
    end = strrchr(s, '\r'); if (end!=NULL) *end = '\0';/* needed for PTI data */
    id = identifyInput(s);
    if ( id==7 ) continue;

/*********** funky fix stuff to get around funky hacked code ***********/
/*    if ( id==5 ) { sscanf(s, "%*35c %3s", trace->IPFversion); continue; }   */
    if ( id==5 ){sscanf(s, "%*35c %3s", trace->IPFversion); id=identifyInput(&s[5]);}
/*********** funky fix stuff to get around funky hacked code ***********/

    if ( id==6 ) { strcpy(trace->IPFversion,"PTI"); *state|=DATA_PTI; continue;}
    if ( id==60) { strcpy(format, SYST_FORMAT_312); continue; }
    if ( id==50) { strcpy(format, COMO_FORMAT_312); continue; }
    if ( id==51) {
        strcpy(format, COMO_FORMAT_314);
        continue;
    }
    if ( id==20) { strcpy(format, OUTG_FORMAT_312); }
    if ( id==21) { strcpy(format, OUTG_FORMAT_PTI); }
    if ( id==30) { strcpy(format, OVLD_FORMAT_312); }
    if ( id==31) { strcpy(format, OVLD_FORMAT_314); }
    if ( id==32) { strcpy(format, OVLD_FORMAT_316); }
    if ( id==33) { strcpy(format, OVLD_FORMAT_PTI); }
    if ( id==40) { strcpy(format, BUSV_FORMAT_312); }
    if ( id==41) { strcpy(format, BUSV_FORMAT_316); }
    if ( id==42) { strcpy(format, BUSV_FORMAT_PTI); }

    if ( (*state & SRCHN_4_COMO_SUM) && (id==10) ) /* toggle from search to */
    {						   /* reading COMO 	    */
        *state &= ~(SRCHN_4_COMO_SUM);
        *state |= READING_COMO_SUM;
        continue;
    }

    if ( (*state & SRCHN_4_OUTG_SUM) && (id==11 || id==13) ) {
        *state &= ~(SRCHN_4_OUTG_SUM);		   /* toggle from search to */
        *state |= READING_OUTG_SUM;		   /* reading OUTAGE summary*/
        continue;
    }

    if ( (*state & SRCHN_4_FAIL_SUM) && (id==12) ) {
        *state &= ~(GET_OVLD | GET_BUSV);	   /* toggle form search to  */
        *state &= ~(SRCHN_4_FAIL_SUM);	       /* reading failed sol summary */
        *state |= READING_FAIL_SUM;
        continue;
    }

    if (id==-1) {
        if ( *state & READING_COMO_SUM ) *state &= ~READING_COMO_SUM;
        if ( *state & READING_OUTG_SUM ) *state &= ~READING_OUTG_SUM;
        if ( *state & READING_FAIL_SUM ) *state &= ~READING_FAIL_SUM;
    }

    if ( !(*state & (READING | SRCHING)) ) {
        return NULL;
    }

    if ( !(*state & READING) ) continue;

    if ( (id==30 || id==31 || id==32 || id==33) && (*state & GET_OUTG) ) {
        *state &= ~GET_BUSV; *state |= GET_OVLD;
    }

    if ( (id==40 || id==41 || id==42) && (*state & GET_OUTG) ) {
        *state &= ~GET_OVLD; *state |= GET_BUSV;
    }

    if ( (id==20 || id==21) && (*state & READING_OUTG_SUM) ) { /* branch/common mode/PTI outage */

        *state &= ~(GET_OVLD | GET_BUSV | GET_OUTG); /* clear all GET_* flags */
        if (*state & DATA_PTI) {
            np = NULL;                    /* no common-mode redundant outages */
        }
        else {              /* only check for redundant outages with IPF data */
            sscanf(s, " %38[^\n]", name);
            np = lookup(name);
        }

        if ( np!=NULL ) {     /* condition satified only if IPF AND redundant */
            if (np->keep == 0) continue;
            if (np->keep == 2) {
                cf_logErr("Redundant Outage Skipped: [%s]\n", name);
                continue;
            }
            cf_freeList(problem);
            problem = getCoMoOutg(s);
        } 
        else {
            cf_freeList(problem);
            problem = getBrchOutg(s, format);
        }

        if (problem!=NULL) *state |= GET_OUTG; /*set only if valid outage found*/

        if (*state & DATA_PTI) {
            *state |= GET_OVLD; strcpy(format, OVLD_FORMAT_PTI);
        }
    }

    if ( (id==0) && (*state & READING_COMO_SUM) ) { /* install: 0-skip, 1-rpt */


        if ( sscanf(s, COMO_MOD_NA_312, name) == 1)
        {

            if (install(name, 0)==NULL) cf_logErr("install(name, 0) failed\n");
        }
        else if ( getCoMoRslt(s, format, *trace) == 1) {
            if (install(name, 1)==NULL) cf_logErr("install(name, 1) failed\n");
        }

        if (trace->redun==REDUN_REMOVE)
            installResult(s); /* screen out outages already taken by com.mode */
    }            /* default is 0, upgrade to keep=1 if any result yields keep */
    if ( (id==0) && (*state & GET_OVLD) ) {
        cf_freeList(problem->next);
        if ( (getOvldLoad(s, format, problem)) == 1) continue;
        *state |= VALID_DATA;
        return problem;
    }

    if ( (id==0) && (*state & GET_BUSV) ) {
        cf_freeList(problem->next);
        if ( (getBusvVolt(s, format, problem)) == 1) continue;
        *state |= VALID_DATA;

        if (*state & DATA_PTI) {
            if (strcmp(format, BUSV_FORM_3_PTI)==0) {
                strcpy(format, BUSV_FORMAT_PTI);
            }
            else if (strcmp(format, BUSV_FORM_2_PTI)==0) {
                if (pos > 0) {
                    fseek(pfo_File, pos, SEEK_SET);
                    strcpy(format, BUSV_FORM_3_PTI);
                }
            }
            else if (strcmp(format, BUSV_FORMAT_PTI)==0) {
                if (pos > 0) { 
                    fseek(pfo_File, pos, SEEK_SET);
                    strcpy(format, BUSV_FORM_2_PTI);
                }
            }
        }
        return problem;
    }

    if ( (id==0) && (*state & READING_FAIL_SUM) ) {
        if ( (problem=getSystProb(s, format, problem))==NULL) continue; 
        *state |= VALID_DATA;
        return problem;
    }
  }

  return NULL;
}

void installResult(char *s)
{
  int convL, convT;
  char n1[9], v1[6], n2[9], v2[6], ckt, own[4], result[39];

  convL = sscanf(s, " L%*3cD%*3c%3[^\n]%*9c%8[^\n]%*c%5[^\n]%*c%8[^\n]%*c%5[^\n]%*c%1c",
    own, n1, v1, n2, v2, &ckt);
  convT = sscanf(s, " T%*3cD%*3c%3[^\n]%*9c%8[^\n]%*c%5[^\n]%*c%8[^\n]%*c%5[^\n]%*c%1c",
    own, n1, v1, n2, v2, &ckt);
  if (convL<5 && convT<5) return;

  sprintf(result, "%-8.8s %5.5s  %-8.8s %5.5s  %c  %-3.3s",
    n1, v1, n2, v2, ckt, own);

  if (install(result, 2)==NULL)    /* 2 indicates a common-mode outage result */
    cf_logErr("install(result, 2) failed\n");
/*  cf_logErr("Result Installed: [%s]\n", result); */
}


int  getCoMoRslt(char *s, char *format, Trace trace)
/***************************************************************************\
* return 1 keep
*        0 do not report this outage 
\***************************************************************************/
{
  char type[3], chng, own[4], z1[3], z2[3];
  float v1, v2;

  if ( sscanf(s, format, type, &chng, own, z1, z2, &v1, &v2) != 7 ) return 0;
  if (type[0]!='B' && type[0]!='T' && type[0]!='L') return 0;
  if (type[0]=='B') { strcpy(z2, z1); v2 = v1; }
  if ( cf_validZone(trace.zoneOutg, z1, z2)==0 ) return 0;
  if ( cf_validOwner(trace.ownrOutg, own)==0 ) return 0;
  if ( cf_validBasekv(trace.bskvOutg, v1, v2)==0 ) return 0;
  return 1; /* report */
}


Link *getSystProb(char *s, char *format, Link *problem)
/****************************************************************************\
* returns: NULL if not valid, Link * if valid 
\****************************************************************************/
{
  cf_Name b, *systData;
  ff_soln bo, *solnData;
  Link *systLink, *solnLink;
  char name[39];
  struct nlist *np;

  sscanf(s, " %38[^\n]", name);
  np = lookup(name);

  if (np==NULL)
    problem = getBrchOutg(s, format);
  else if (np->keep==1)
    problem = getCoMoOutg(s);
  else
    return NULL;

  systLink = cf_addLink(problem, sizeof(cf_Name));
  systData = (cf_Name *) systLink->data;

  strcpy(b.type, "sp");                            /* solution problems */
  memcpy(systData, &b, sizeof(cf_Name));

  if (strstr(s, "separation")!=NULL)
    bo.code = 'S';
  else if (strstr(s, "(Reactive)" )!=NULL)
    bo.code = 'X';
  else if (strstr(s, "No solution")!=NULL)
    bo.code = 'F';
  else
    return NULL;

  solnLink = cf_addLink(systLink, sizeof(ff_soln));

  solnData = (ff_soln *) solnLink->data;
  memcpy(solnData, &bo, sizeof(ff_soln));

  return problem;
}

int  getBusvVolt(char *s, char *format, Link *problem)
/****************************************************************************\
*
\****************************************************************************/
{
  cf_Bus b, *busvData;
  ff_soln bo, *solnData;
  Link *busvLink, *solnLink;

  memset(&b, '\0', sizeof(cf_Bus));
  memset(&bo, '\0', sizeof(ff_soln));

  if (strncmp(format, BUSV_FORMAT_PTI, 5)==0) {
    if ( sscanf(s, format, b.name, &b.kv, &bo.ratio) < 3) return 1;
    bo.actual = bo.ratio * b.kv;
  }
  else {
    if ( sscanf(s, format, b.name, &b.kv, b.owner, b.zone, &bo.actual, &bo.ratio,
        &bo.pre_con, &bo.min_ref, &bo.max_new) < 8 ) return 1;
    if (bo.max_new==0) {          /* shift over values for old format, <v.316 */
        bo.max_new = bo.min_ref;  bo.min_ref=bo.pre_con;  bo.pre_con = -9999;
    }
  }

  strcpy(b.type, "B ");

  busvLink = cf_addLink(problem, sizeof(cf_Bus));
  solnLink = cf_addLink(busvLink, sizeof(ff_soln));

  busvData = (cf_Bus *) busvLink->data;
  solnData = (ff_soln *) solnLink->data;

  memcpy(busvData, &b, sizeof(cf_Bus));
  memcpy(solnData, &bo, sizeof(ff_soln));

  return 0;
}


int  getOvldLoad(char *s, char *format, Link *problem)
/****************************************************************************\
*
\****************************************************************************/
{
  cf_Branch r, *ovldData;
  ff_soln ro, *solnData;
  Link *ovldLink, *solnLink;
  
  memset(&r, '\0', sizeof(cf_Branch));
  memset(&ro, '\0', sizeof(ff_soln));

  if (strncmp(format, OVLD_FORMAT_PTI, 5)==0) {
    if (sscanf(s, format, r.bus1_name, &r.bus1_kv, r.bus2_name, &r.bus2_kv,
        &r.ckt_id, &ro.pre_con, &ro.actual, &ro.max_new, &ro.ratio)<9) return 1;
  }
  else {
    if (sscanf(s, format, r.bus1_name, &r.bus1_kv, r.bus2_name, &r.bus2_kv,
        &r.ckt_id, &r.section, r.owner, r.bus1_zone, r.bus2_zone, &ro.max_new,
        &ro.code, &ro.ratio, &ro.actual, &ro.pre_con, ro.date) < 13) return 1;

    if (ro.pre_con==0)
      ro.pre_con = -9999;
    ro.ratio = 100 * ro.ratio; /* per cent = 100 * Per Unit */
    date(&ro.date[0]);
  }

  if (r.bus1_kv==r.bus2_kv)
    strcpy(r.type, "L ");
  else
    strcpy(r.type, "T ");

  ovldLink = cf_addLink(problem, sizeof(cf_Branch));
  solnLink = cf_addLink(ovldLink, sizeof(ff_soln));
  ovldData = (cf_Branch *) ovldLink->data;
  solnData = (ff_soln *) solnLink->data;

  memcpy(ovldData, &r, sizeof(cf_Branch));
  memcpy(solnData, &ro, sizeof(ff_soln));
  return 0;
}

Link *getCoMoOutg(char *s)
/****************************************************************************\
*
\****************************************************************************/
{
  cf_Name *comoData;
  Link *comoLink;
  char  name[CF_INBUFSIZE];

  if (sscanf(s, " %38[^\n]", name) < 1) return NULL;

  comoLink = cf_newLink(sizeof(cf_Name));

  if (comoLink==NULL || comoLink->data==NULL) return NULL;

  comoLink->kind = CF_KIND_NAME;
  comoData = (cf_Name *) comoLink->data;

  strcpy(comoData->type, "cm");
  sprintf(comoData->name, "%-38s", name);

/******************** missing element in common mode stuff *******************/
            if( strstr( s, "MODE ELEMENT" ) )
            {
              comoData->me_flag = 1;
            }
            else
              comoData->me_flag = 0;
/******************** missing element in common mode stuff *******************/

  return comoLink;
}


Link *getBrchOutg(char *s, char *format)
{
  cf_Branch *brznData, t;
  Link      *brznLink;

  memset(&t, '\0', sizeof(cf_Branch));
  if (sscanf(s, format, t.bus1_name, &t.bus1_kv, t.bus2_name, &t.bus2_kv,
    &t.ckt_id, t.owner, t.bus1_zone, t.bus2_zone) < 8) return NULL;
  brznLink = cf_newLink(sizeof(cf_Branch));

  if ( t.bus1_zone[0]=='-' || t.bus1_zone[1]=='-' ) t.bus1_zone[0] = '\0'; 
  if ( t.bus2_zone[0]=='-' || t.bus2_zone[1]=='-' ) t.bus2_zone[0] = '\0'; 
  if ( t.owner[0]=='-' ) t.owner[0] = '\0'; 

  if (t.bus1_kv==t.bus2_kv)
    strcpy(t.type, "L ");
  else
    strcpy(t.type, "T ");

  brznData = (cf_Branch *) brznLink->data;
  memcpy(brznData, &t, sizeof(cf_Branch));

  return brznLink;
}

int identifyInput(char *s)
/****************************************************************************\
*
\****************************************************************************/
{
  if (strstr(s, BPA_PF_PROG_VERS) != NULL) return  5; /* 5 = program version  */
  if (strstr(s, PTI_INTERAC_PSSE) != NULL) return  6; /* 6 = PTI program */
  if (strstr(s, HEADER_BREAK_PTI) != NULL) return  7; /* 7 = page break */
  if (strstr(s, HEADER_COMME_PTI) != NULL) return  7; /* 7 = header comment */
  if (strstr(s, DIS_FAC_FILE_PTI) != NULL) return  7; /* 7 = misc. data files */
  if (strstr(s, SUB_DES_FILE_PTI) != NULL) return  7; /* 7 = misc. data files */
  if (strstr(s, MON_ELE_FILE_PTI) != NULL) return  7; /* 7 = misc. data files */
  if (strstr(s, CON_DES_FILE_PTI) != NULL) return  7; /* 7 = misc. data files */
  if (strstr(s, CONTINGENCY_ANAL) != NULL) return  7; /* 7 = page break */
  if (strstr(s, OUTG_SIMUL_INPUT) != NULL) return  7; /* 7 = page break */
  if (strstr(s, __O_U_T_A_G_E__ ) != NULL) return  7; /* 7 = page header */
  if (strstr(s, "Mode  Name    ") != NULL) return  7; /* 7 = old failed sol r */
  if (strstr(s, "Results in:"   ) != NULL) return  7; /* 7 = common mode outg */
  if (strstr(s, " --- "         ) != NULL) return  7; /* 7 = common mode list */
  if (strstr(s, "(MW)   (MVAR) ") != NULL) return  7; /* 7 = common mode list */
  if (strstr(s, " MW     MVAR  ") != NULL) return  7; /* 7 = common mode list */
  if (strstr(s, "Failed reactiv") != NULL) return  7; /* Added 4/27 wer */
  if (strlen(s) <= 3)                      return  7; /* 7 = blank line */
  if (strstr(s, SUM_COM_MODE_OUT) != NULL) return 10; /*10 = common mode outg */
/**************** debug Stuff ******************/
  if (strstr(s, SUM_COM_MODE_OUT) != NULL) { 
     { printf("\n SUM_COM_MODE_OUT has been found \n"); return 10;}
  }
/**************** debug Stuff ******************/

  if (strstr(s, SUM_PROBS_4E_OUT) != NULL) return 11; /*11 = outage report */
  if (strstr(s, SUM_SYST_SEP_DIV) != NULL) return 12; /*12 = failed soln rept */
  if (strstr(s, CONTIN_EVENT_PTI) != NULL) return 13; /*13 = outage report,PTI*/
  if (strstr(s, "Summary of"    ) != NULL) return -1; /*-1 = wrong summary */
  if (strstr(s, "- - - - - - - ") != NULL) return 20; /*20 = outage */
  if (strstr(s, OPEN_LINE_FR_PTI) != NULL) return 21; /*21 = outage, PTI */
  if (strstr(s, OVLD_RAT_TY_312 ) != NULL) return 30; /*30 = overload follows */
  if (strstr(s, OVLD_RAT_TY_314 ) != NULL) return 31; /*31 = ovld new format */
  if (strstr(s, OVLD_RAT_TY_316 ) != NULL) return 32; /*32 = ovld newest form*/
  if (strstr(s, OVLD_FRO_TO_PTI ) != NULL) return 33; /*33 = ovld PTI format */
  if (strstr(s, BUS_VV_A_PU_312 ) != NULL) return 40; /*40 = old bus volt form*/
  if (strstr(s, BUS_VV_A_PU_316 ) != NULL) return 41; /*41 = new bus volt form*/
  if (strstr(s, BUS_BUS_VPU_PTI ) != NULL) return 42; /*42 = PTI bus volt form*/
  if (strstr(s, TY_CH_OWN_B_312 ) != NULL) return 50; /*50 = old co mo format */
  if (strstr(s, TY_CH_OWN_Z_315 ) != NULL) return 51; /*51 = new co mo format */
  if (strstr(s, OUTG_ID_OWN_314 ) != NULL) return 60; /*60 = failed soln rept */
  if (strstr(s, OUTG_ID_OWN_312 ) != NULL) return 60; /*60 = old failed sol r */
  return 0;                                           /* 0 = possible data */
}

void buildReport(Trace *trace, cf_Out *rpt)
/*****************************************************************************\
* build table for output report
*  	 ( read trace->mainTable, builds trace->rptTable )
\*****************************************************************************/
{
  Link      *curRow, *mainRow, *newRow, *probLink, *outgLink, *solnLink;
  cf_Name   *probData;
  cf_Branch *outgData;
  int        Dbug = 1;

  printf(" Building Report Table\n");

  trace->rptTable = NULL;
  curRow = NULL;

  /* Loop through all the outages in mainTable */

  for(mainRow=trace->mainTable; mainRow!=NULL; mainRow=mainRow->next)
  {
    outgLink = (Link *) mainRow->data;
    probLink = (outgLink!=NULL) ? outgLink->next : NULL;
    solnLink = (probLink!=NULL) ? probLink->next : NULL;
    probData = (probLink!=NULL) ? (cf_Name *) probLink->data : NULL;

    outgData = (cf_Branch *) outgLink->data;

    if (Dbug==1) {
        fprintf(stderr, "\noutg: %s%5.1f %s%5.1f %c",
            outgData->bus1_name, outgData->bus1_kv,
            outgData->bus2_name, outgData->bus2_kv, outgData->ckt_id);
    }

    if (probData==NULL) continue;

    if (probData->type[0]!='s') {
        if (rpt->type==TYPE_OUTG_OVLD || rpt->type==TYPE_OVLD_OUTG) {     /* outg, ovld */
            if ( validLoad(*trace, solnLink)==0 ) continue;
        }
        else if (rpt->type==TYPE_OUTG_BUSV || rpt->type==TYPE_BUSV_OUTG) {/* outg, busv */
            if (solnLink->prev!=probLink) printf("uggg!\n");
            if (probLink->next!=solnLink) printf("bugg!\n");
            if ( validVolt(*trace, solnLink)==0 ) continue;
        }
    }
    else if ( validSoln(solnLink, trace )==0)
      continue;                   /* skip if 0 */

    if (rpt->diff==VARI_YES) {
        if (!validVariance(*trace, solnLink)) continue;
    }

/*************************************************************************/
    newRow = cf_link2row(outgLink);

    if (rpt->type==TYPE_OVLD_OUTG || rpt->type==TYPE_BUSV_OUTG)
        swapRowIdLinks((Link *) newRow->data);

    if (trace->sort==SORT_BY_INPUT) {
        cf_insLink(&(trace->rptTable), &curRow, newRow, (long) trace,
            CF_INS_SKIP, (int (*)(Link*, Link*, int))(ff_srtByInput));
    }
    else
    {
        cf_insLink(&(trace->rptTable), &curRow, newRow, (long) trace->sort,
            CF_INS_SKIP, (int (*)(Link*, Link*, int))(ff_cmpRow));
    }

    if (Dbug==1) {
        fprintf(stderr, " valid: %s%5.1f %s%5.1f %c",
            outgData->bus1_name, outgData->bus1_kv,
            outgData->bus2_name, outgData->bus2_kv, outgData->ckt_id);
    }
  }
  return;
}

void swapReport(Trace *trace)
/***************************************************************************\
*  build table for output report 
*
\***************************************************************************/
{
  Link *curRow, *rptRow, *tmpRow, *tmpTable;

  if (trace->aux->type==TYPE_NONE) return;
  printf(" Swapping Report Table\n");

  curRow = NULL;
  rptRow = trace->rptTable;
  tmpTable = NULL;
  while(rptRow!=NULL) {
    tmpRow = rptRow;
    if (rptRow->prev!=NULL) rptRow->prev->next = rptRow->next;
    if (rptRow->next!=NULL) rptRow->next->prev = rptRow->prev;
    rptRow = rptRow->next;

    tmpRow->next = NULL;
    tmpRow->prev = NULL;

    swapRowIdLinks((Link *) tmpRow->data);
    if (trace->sort==SORT_BY_INPUT) {
        cf_insLink(&(tmpTable), &curRow, tmpRow, (long) trace,
            CF_INS_SKIP, (int (*)(Link*, Link*, int))(ff_srtByInput));
    }
    else {
        cf_insLink(&(tmpTable), &curRow, tmpRow, (long) trace->sort,
            CF_INS_SKIP, (int (*)(Link*, Link*, int))(ff_cmpRow));
    }
  }
  trace->rptTable = tmpTable;
  return;
}

void swapRowIdLinks(Link *idLink)
{ /* swaps only the pointers to the Id objects */
  Link *tempLink;
  tempLink = (Link *) idLink->data;
  idLink->data = idLink->next->data;
  idLink->next->data = tempLink;
}

void ff_appSoln2Row(Link *curRow, Link *solnLink, int skip)
{
  Link *linkPtr;
  linkPtr  = (Link *) curRow->data;
  while (skip-- > 0) {
    if (linkPtr->next==NULL) {
        linkPtr->next = cf_newLink(0);
        if (linkPtr->next!=NULL) linkPtr->next->prev=linkPtr;
    }
    linkPtr = linkPtr->next;
  }
  linkPtr->next = solnLink;
  solnLink->prev = linkPtr;
}

int ff_cmpID(Link *newLink, Link *xstLink)
{
  cf_Branch *newIdData, *xstIdData;
  if (newLink==NULL || xstLink==NULL) return 1;
  if (newLink->data==NULL || xstLink->data==NULL) return -1;
  newIdData = (cf_Branch *) newLink->data;
  xstIdData = (cf_Branch *) xstLink->data;
  if (newIdData->type[0]=='c') {                        /* common_mode_outage */
    return ff_cmpName((cf_Name *) newIdData, (cf_Name *) xstIdData);
  }
  else if (newIdData->type[0]=='B') {                         /* bus u/o volt */
    return cf_cmpBus((pf_AC_bus *) newIdData, (pf_AC_bus *) xstIdData);
  }
  else if (newIdData->type[0]=='s') {                       /* system problem */
    return (newIdData->type[0] - xstIdData->type[0]);
  }
  else {                                /* branch outage or overloaded branch */
    return cf_cmpBranch((pf_branch *) newIdData, (pf_branch *) xstIdData);
  }
}

int ff_cmpType(Link *newId, Link *xstId)
{
  cf_Name *newType, *xstType;
  newType = (cf_Name *) newId->data;
  xstType = (cf_Name *) xstId->data;
  if ( newType->type[0] < xstType->type[0] ) return -1;        /* [type]-sort */
  if ( newType->type[0] > xstType->type[0] ) return  1;        /* [type]-sort */
  if ( newType->type[0]=='s' ) return 1;          /* force sort with ff_srtID */
  return 0;
}

int ff_cmpName(cf_Name *b1, cf_Name *b2)
{
  float c;
  if ( c=strncmp(b1->type, b2->type, 1) ) return c>0 ? 1 : -1;
  if ( c=strcmp(b1->name, b2->name) )     return c>0 ? 1 : -1;
  return 0;
}

int ff_cmpRow(Link *newRow, Link *xstRow, int sort)
{
  int srt;
  Link *newId, *xstId;

  newId = (Link *) newRow->data;
  xstId = (Link *) xstRow->data;

  if (sort==SORT_BY_SOLN || sort==SORT_BY_DIFF) {
    if ( (srt=ff_cmpType(newId, xstId)) !=0 ) return srt;
    if ( (srt=ff_srtSoln(newId->next->next, xstId->next->next, sort)) != 0 )
        return srt;
  }
  if ( (srt=ff_srtID(newId, xstId, sort)) != 0 ) return srt;
  else return ff_srtID(newId->next, xstId->next, sort);
}

int ff_srtSoln(Link *newSoln, Link *xstSoln, int sort)
{ /* compares only the solution in the first case for sorting purposes */
  ff_soln *newData, *xstData;
  float    newValu,  xstValu;

  if (newSoln==NULL && xstSoln==NULL) return  0;
  if (newSoln==NULL && xstSoln!=NULL) return  1;
  if (newSoln!=NULL && xstSoln==NULL) return -1;

  if (newSoln->next==NULL && xstSoln->next==NULL) return  0;
  if (newSoln->next==NULL && xstSoln->next!=NULL) return  1;
  if (newSoln->next!=NULL && xstSoln->next==NULL) return -1;
/* also could be called SORT_BY_LAST */
  if (sort==SORT_BY_DIFF) { /* newSoln->refSoln->delSoln: delSoln may be NULL */
    while (newSoln!=NULL && newSoln->next!=NULL) newSoln=newSoln->next;
    while (xstSoln!=NULL && xstSoln->next!=NULL) xstSoln=xstSoln->next;
    newData = (ff_soln *) ( newSoln!=NULL ? newSoln->data : NULL );
    xstData = (ff_soln *) ( xstSoln!=NULL ? xstSoln->data : NULL );
  }
  else {
    newData = (ff_soln *) newSoln->data;
    xstData = (ff_soln *) xstSoln->data;
  }
  newValu = (newData!=NULL) ? newData->ratio : 0;
  xstValu = (xstData!=NULL) ? xstData->ratio : 0;
  if (newValu==0 && xstValu==0) return 0;
  if (newValu < xstValu) return  1;
  if (newValu > xstValu) return -1;
  return 0;
}

int ff_srtID(Link *newLink, Link *xstLink, int sort)
{
  cf_Branch *newIdData, *xstIdData;

  if (newLink==NULL || xstLink==NULL) return 1;
  if (newLink->data==NULL || xstLink->data==NULL) return -1;
  newIdData = (cf_Branch *) newLink->data;
  xstIdData = (cf_Branch *) xstLink->data;
  if (newIdData->type[0]=='c') {                        /* common_mode_outage */
    return ff_cmpName((cf_Name *) newIdData, (cf_Name *) xstIdData);
  }
  else if (newIdData->type[0]=='B') {                         /* bus u/o volt */
    return ff_srtBus((pf_AC_bus *) newIdData, (pf_AC_bus *) xstIdData, sort);
  }
  else if (newIdData->type[0]=='s') {                       /* system problem */
    return (newIdData->type[0] - xstIdData->type[0]);
  }
  else {                                /* branch outage or overloaded branch */
    return ff_srtBranch((cf_Branch *) newIdData, (cf_Branch *) xstIdData, sort);
  }
}

int ff_srtBus(pf_AC_bus *b1, pf_AC_bus *b2, int sort) /* sort bus */
{
  int c;
  float d;

  if ( (c=strcmp(b1->type, b2->type)) != 0 ) return c;    /* [type]-sort     */
  if (sort==5) return 1;                                  /* [input order]   */
  if (sort==4) {                                          /* [zone]-alpha-kv */
    if ( (c=strcmp(b1->zone, b2->zone)) != 0 ) return c;
  }
  else if (sort==3) {                                     /* [owner]-alpha-kv */
    if ( (c=strcmp(b1->owner, b2->owner)) != 0 ) return c;
  }
  else if (sort==2) {                                     /* [kv]-alpha-kv */
    d = (b1->kv - b2->kv);
    if ( fabs(d) > .001 ) return d;
  }
  return cf_cmpBus(b1, b2);                               /* [alpha-kv] */
}

int ff_srtByInput(Link *newRow, Link *xstRow, void *sort)
{
  int type;
  int index1, index2;   /* 1 - first Link, 2 - second Link (first Link->next) */
  Link *newRowId, *list1, *list2, *newId1, *newId2, *xstId1, *xstId2;
  cf_Branch *newOutgData, *newProbData;
  Trace *trace;

  trace = (Trace *) sort;
  type = trace->rpt->type;
  if (type==TYPE_OUTG_OVLD || type==TYPE_OUTG_BUSV) {
    newRowId    = (Link *)      newRow->data;
    newOutgData = (cf_Branch *) newRowId->data;
    newProbData = (cf_Branch *) newRowId->next->data;
    list1 = (newOutgData->type[0]=='c') ? trace->comoList : trace->outgList;
    list2 = (newProbData->type[0]=='B') ? trace->busvList : trace->ovldList;
  }
  else if (type==TYPE_OVLD_OUTG || type==TYPE_BUSV_OUTG) {
    newRowId    = (Link *)      newRow->data;
    newProbData = (cf_Branch *) newRowId->data;
    newOutgData = (cf_Branch *) newRowId->next->data;
    list1 = (newProbData->type[0]=='B') ? trace->busvList : trace->ovldList;
    list2 = (newOutgData->type[0]=='c') ? trace->comoList : trace->outgList;
  }

  newId1 = (Link *) newRow->data;
  newId2 = (Link *) newId1->next;
  xstId1 = (Link *) xstRow->data;
  xstId2 = (Link *) xstId1->next;
  index1 = getIndex((Link *) newId1, (Link *) xstId1, list1);
  if (index1!=0) return index1;
  index2 = getIndex((Link *) newId2, (Link *) xstId2, list2);

  return index2;
}

int getIndex(Link *newLink, Link *xstLink, Link *list)
/****************************************************************************\
* determine which branch/bus comes first in the input list
* uses a start-at-beginning-and-search-whole-list approach
\****************************************************************************/
{
  if (list==NULL) return ff_cmpID(newLink, xstLink);    /* default [alpha-kv] */
  if (ff_cmpID(newLink, xstLink)==0) return 0;     /* newLink same as xstLink */
  for (; list!=NULL; list=list->next) {
    if (ff_cmpID(list, newLink)==0) return -1;     /* switching return values */
    if (ff_cmpID(list, xstLink)==0) return  1;     /* reverses the sort order */
  }

  return  1;           /* +1 & -1 seem to have same result for return value */
}


void printReport(Trace *trace, cf_Out *rpt)
/***************************************************************************\
*
* Called by: main
\***************************************************************************/
{
  int      i, doN, skipN, cmp;
  Link    *curRow, *curID, *curSoln, *refID, *pfoPtr, *curPfo;

  if (rpt->type==TYPE_NONE) return;

  openReport(rpt);

  skipN  = 0;
  curPfo = trace->pfoList;
  if (curPfo==NULL || trace->rptTable==NULL) {
    fprintf(rpt->file, "*** Warning: No Data Found! ***");
  }

  for (pfoPtr=curPfo;curPfo!=NULL && trace->rptTable!=NULL;curPfo=pfoPtr)
  {
    for (doN=0; pfoPtr!=NULL && doN<rpt->wide; pfoPtr=pfoPtr->next) doN++;

    printHeader(rpt, curPfo, trace );

    for (curRow = trace->rptTable; curRow != NULL; curRow=curRow->next)
    {
      if ( rpt->form!=FORM_DATA_FULL && rpt->line==0 )
            printHeader(rpt, curPfo, trace );
      curID = (Link *) curRow->data;
      curSoln = curID->next->next;

      for (i = skipN; curSoln!=NULL && i-- > 0; curSoln = curSoln->next);

      refID = (curRow->prev!=NULL) ? (Link *) curRow->prev->data : NULL;
      cmp = ff_cmpID(curID, refID);
      if (rpt->form==FORM_DATA_FULL || rpt->form==FORM_DATA_BREF || cmp) {
            printID(rpt, curID, 1);
        }

      if (rpt->form!=FORM_DATA_FULL && rpt->form!=FORM_DATA_BREF && cmp) {
            fprintf(rpt->file, "\n"); rpt->line++;
      }
      if (rpt->form==FORM_PAGE_WIDE) {
            printID(rpt, NULL,  1);
      }
      printID(rpt, curID->next, 2);

      chkBranch (trace, curID);  /* deLink branches with overloads from  No O/L Log */

      printSoln(rpt, curID, curSoln, doN);

      refID = (curRow->next!=NULL) ? (Link *) curRow->next->data : NULL;
      cmp = ff_cmpID(curID, refID);
      if (rpt->form!=FORM_DATA_FULL && rpt->form!=FORM_DATA_BREF && cmp) {
          fprintf(rpt->file, "\n"); rpt->line++;
      }
      if (rpt->form!=FORM_DATA_FULL && rpt->form!=FORM_DATA_BREF) {
            pageFooter(rpt, curRow->next, pfoPtr);
      }
    }

    chkBranchLog (trace, curPfo);  /* send No O/L Log to cf_logErr */       

    skipN += doN;
  }
}

void chkBranch(Trace *trace, Link *curID)
/**************************************************************************\
* deLink branches with overloads from  No O/L Log 
*
\**************************************************************************/
{
  cf_Branch *ovldData, *chkBranchData;        /*  No O/L Log support */
  Link    *chkBranchStep, *chkBranchOvld;     /*  No O/L Log support */

  chkBranchOvld = curID->next;
  ovldData = (cf_Branch *)chkBranchOvld->data;
  chkBranchStep = trace->chkBranchStart;                /* dmstest */

  while (chkBranchStep!=NULL) {
    chkBranchData = (cf_Branch *)chkBranchStep->data;
    if  (ff_cmpID(chkBranchStep, chkBranchOvld) == 0) {
         chkBranchStep = cf_delLink(&trace->chkBranchStart, chkBranchStep);
    }
    else {            
      chkBranchStep = chkBranchStep->next;
    }
  }
  return;
}

void chkBranchLog(Trace *trace, Link *curPfo)
/* send No O/L Log to cf_logErr */

{
  int      cmp;
  cf_Branch *ovldData, *chkBranchData;        /*  No O/L Log support */
  Link    *chkBranchStep, *chkBranchOvld;     /*  No O/L Log support */

        chkBranchStep = trace->chkBranchStart;
        while (chkBranchStep!=NULL) {
          chkBranchData = (cf_Branch *)chkBranchStep->data;
          cf_logErr(" %9s %7.2f %9s %7.2f %c is not overloaded in %s\n",
          chkBranchData->bus1_name, chkBranchData->bus1_kv,
          chkBranchData->bus2_name, chkBranchData->bus2_kv,
	  chkBranchData->ckt_id,
          curPfo->data);
          chkBranchStep = chkBranchStep->next;
        }
  return;
}


void printHeader(cf_Out *rpt, Link *curPfo, Trace *trace)
{

  fprintf( rpt->file, ".FINDOUT VERSION %3d (%8s)\n", VERSN_NUM, VERSN_DATE );
  rpt->line += 1;

  if( trace->trc->diff == VARI_YES )
  {
    fprintf( rpt->file, ".VARIANCE CEILING =%7.4f  VARIANCE FLOOR =%7.4f\n",
      trace->varCeiling, trace->varFloor );
    rpt->line += 1;
  }

  if( trace->rpt->type==TYPE_OUTG_OVLD || trace->rpt->type==TYPE_OVLD_OUTG )
  {
    fprintf( rpt->file, ".Minimum loading to print (MIN_LOAD_PCT) =%9.2f\n",
      trace->minLoadPct );
    rpt->line +=1;
  }

  else if(trace->rpt->type==TYPE_OUTG_BUSV || trace->rpt->type==TYPE_BUSV_OUTG)
  {
    fprintf( rpt->file, ".Max/Min 500kv PU to print = (%6.4f  %6.4f)\n",
      trace->max5, trace->min5 );
    fprintf( rpt->file, ".Max/Min    kv PU to print = (%6.4f  %6.4f)\n",
      trace->maxV, trace->minV );
    rpt->line +=2;
  }


       if (rpt->type==TYPE_OUTG_OVLD) printOtBrHeader(rpt, curPfo);
  else if (rpt->type==TYPE_OUTG_BUSV) printOtBsHeader(rpt, curPfo);
  else if (rpt->type==TYPE_OVLD_OUTG) printOvldHeader(rpt, curPfo);
  else if (rpt->type==TYPE_BUSV_OUTG) printBusvHeader(rpt, curPfo);
  return;
}


void printID(cf_Out *rpt, Link *curID, int position)
{
  if (rpt->type==TYPE_OUTG_OVLD) {
    if (position==1) printOutg(rpt->file, curID, rpt->form);
    if (position==2) printOvld(rpt->file, curID, rpt->form);
  }

  if (rpt->type==TYPE_OUTG_BUSV) {
    if (position==1) printOutg(rpt->file, curID, rpt->form);
    if (position==2) printBusv(rpt->file, curID, rpt->form);
  }

  if (rpt->type==TYPE_OVLD_OUTG) {
    if (position==1) printOvld(rpt->file, curID, rpt->form);
    if (position==2) printOutg(rpt->file, curID, rpt->form);
  }

  if (rpt->type==TYPE_BUSV_OUTG) {
    if (position==1) printBusv(rpt->file, curID, rpt->form);
    if (position==2) printOutg(rpt->file, curID, rpt->form);
  }
}
void printSoln(cf_Out *rpt, Link *curID, Link *curSoln, int doN)
{
  cf_Name *data;
       if (rpt->type==TYPE_OUTG_OVLD) data = (cf_Name *) curID->next->data;
  else if (rpt->type==TYPE_OUTG_BUSV) data = (cf_Name *) curID->next->data;
  else if (rpt->type==TYPE_OVLD_OUTG) data = (cf_Name *) curID->data;
  else if (rpt->type==TYPE_BUSV_OUTG) data = (cf_Name *) curID->data;

  if (data->type[0]=='s') {
    printProb(rpt, curSoln, doN);
  }
  else if (rpt->type==TYPE_OUTG_OVLD || rpt->type==TYPE_OVLD_OUTG) {
    printLoad(rpt, curSoln, doN);
  }
  else if (rpt->type==TYPE_OUTG_BUSV || rpt->type==TYPE_BUSV_OUTG) {
    printVolt(rpt, curSoln, doN);
  }
  return;
}

void pageFooter(cf_Out *rpt, Link *nxRow, Link *nxPfo)
{
  if (nxRow!=NULL) {
    ff_printPageFooter(rpt, 3);
  }
  else if (nxRow==NULL && nxPfo!=NULL) {
    ff_printPageFooter(rpt, 9);
    fprintf(rpt->file, "%s\n", DOT_LINE);
    rpt->line++;
    ff_printPageFooter(rpt, FF_PAGE_LENGTH);
  }
  else if (nxRow==NULL && nxPfo==NULL) {
    ff_printPageFooter(rpt, 10);
    tableLegend(rpt);
    ff_printPageFooter(rpt, FF_PAGE_LENGTH);
  }
}
int ff_printPageFooter(cf_Out *rpt, int m)
/* Guarantee room for m lines before page break */
{
  int n;

  n = cf_cntchr(CF_PAGE_FOOTER, '\n');
  rpt->line = Cf_imod(rpt->line, FF_PAGE_LENGTH);
  if ( (FF_PAGE_LENGTH - n - rpt->line) > m ) return 0;

  ff_printBlankLines(rpt, n);
  fprintf(rpt->file, CF_PAGE_FOOTER, rpt->time, rpt->page);
  rpt->line = Cf_imod(rpt->line + n, FF_PAGE_LENGTH);
  rpt->page++;
  return 1;
}
void ff_printBlankLines(cf_Out *rpt, int n)
{
  while (rpt->line < FF_PAGE_LENGTH - n) {
    fprintf(rpt->file, "\n");
    rpt->line++;
  }
  return;
}

void printOtBrHeader(cf_Out *rpt, Link *pfo)
/****************************************************************************\
*
\****************************************************************************/
{ /* COULD USE DEFINE STATEMENTS */
  char *HdLd[] = { /* lead */
    ".|   |OUTAGES CAUSING OVERLOADS| |  |  | |   |      CASE NAME: ->      | | |  |  |    |   |",
    ".|   |                         | |  |  | |   |                         | | |  |  |    |   |",
    ".|   |    Outaged  Branches    | |  |  | |   |   Overloaded Branches   | | |  |  |CRTC|IN |",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|Z1|Z2|T|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|S|Z1|Z2|RATG|MYY|",
    " ",
    ".|   |OUTAGES CAUSING OVERLOADS| | |   |      CASE NAME: ->      | |    |",
    ".|   |                         | | |   |                         | |    |",
    ".|   |    Outaged  Branches    | | |   |   Overloaded Branches   | |CRTC|",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|T|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|RATG|",
    " ",
    ".|   |OUTAGES CAUSING OVERLOADS| | |  |  |    |   |",
    ".|   | Outage followed by...   | | |  |  |    |   |",
    ".|   |    ...List of Overloads | | |  |  |CRTC|IN |",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|S|Z1|Z2|RATG|MYY|",
    " ",
  };

  char *HdRp[] = { /* repeat */
    "%16.16s||||",
    "                ||||",
    " PRE  | POST |POST||",
    " LOAD | LOAD |(%)|F|",
    " ",
    "%4.4s||",
    "    ||",
    "POST||",
    "(%)|F|",
    " ",
  };

  char *HdDf[] = { /* difference */
    "", "", "", "", "",
    "       |       |",
    "POST(%)|POST(%)|",
    "VARIANC|VARIANC|",
    "   UP  |  DOWN |",
    " ",
  };

  int lf, rf, df;

  if (rpt->form==FORM_PAGE_WIDE) lf = 0;
  if (rpt->form==FORM_DATA_FULL) lf = 0;
  if (rpt->form==FORM_DATA_BREF) lf = 5;
  if (rpt->form==FORM_PAGE_COMP) lf = 10;
  rf = (rpt->form==FORM_DATA_BREF) ? 5 : 0;
  df = (rpt->diff==VARI_YES) ? 5 : 0;
  printTableCases(rpt,  pfo, HdLd[0+lf], HdRp[0+rf], HdDf[0+df]);
  printTableHeader(rpt, pfo, HdLd[1+lf], HdRp[1+rf], HdDf[1+df]);
  printTableHeader(rpt, pfo, HdLd[2+lf], HdRp[2+rf], HdDf[2+df]);
  printTableHeader(rpt, pfo, HdLd[3+lf], HdRp[3+rf], HdDf[3+df]);
  printTableHeader(rpt, pfo, HdLd[4+lf], HdRp[4+rf], HdDf[4+df]);
  rpt->line += 5;
  return;
}

void printOtBsHeader(cf_Out *rpt, Link *pfo)
/****************************************************************************\
*
\****************************************************************************/
{
  char *HdLd[] = { /* lead */
    ".|   |OUTAGE CAUSING UO VOLTAGE| |  |  | |   |CASE NAME:->|  |     |     |",
    ".|   |                         | |  |  | |   |            |  |LOWER|UPPER|",
    ".|   |     Outaged  Branch     | |  |  | |   |U/O  Voltage|  |LIMIT|LIMIT|",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|Z1|Z2|T|OWN|< BUS1 ><V1>|ZN|( PU)|( PU)|",
    " ",
    ".|   |OUTAGE CAUSING UO VOLTAGE| | |   |CASE NAME:->|  |",
    ".|   |                         | | |   |            |  |",
    ".|   |     Outaged  Branch     | | |   |U/O  Voltage|  |",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|T|OWN|< BUS1 ><V1>|ZN|",
    " ",
    ".|   |OUTAGE CAUSING UO VOLTAGE| | |  |  |     |     |",
    ".|   | Outage followed by...   | | |  |  |LOWER|UPPER|",
    ".|   |       ...List of Busses | | |  |  |LIMIT|LIMIT|",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|S|Z1|Z2|( PU)|( PU)|",
    " ",
  };

  char *HdRp[] = { /* repeat */
    "%16.16s|||",
    "                |||",
    "ACTUA|ACTUA|CHANGE|",
    "( KV)|( PU)|( PU )|",
    " ",
    "%5.5s|",
    "     |",
    "ACTUA|",
    "( PU)|",
    " ",
  };

  char *HdDf[] = { /* difference */
    "", "", "", "", "",
    "       |       |",
    "VARIANC|VARIANC|",
    "  UP   | DOWN  |",
    "(  PU )|(  PU )|",
    " ",
  };

  int lf, rf, df;

  if (rpt->form==FORM_PAGE_WIDE) lf = 0;
  if (rpt->form==FORM_DATA_FULL) lf = 0;
  if (rpt->form==FORM_DATA_BREF) lf = 5;
  if (rpt->form==FORM_PAGE_COMP) lf = 10;
  rf = (rpt->form==FORM_DATA_BREF) ? 5 : 0;
  df = (rpt->diff==VARI_YES) ? 5 : 0;
  printTableCases(rpt,  pfo, HdLd[0+lf], HdRp[0+rf], HdDf[0+df]);
  printTableHeader(rpt, pfo, HdLd[1+lf], HdRp[1+rf], HdDf[1+df]);
  printTableHeader(rpt, pfo, HdLd[2+lf], HdRp[2+rf], HdDf[2+df]);
  printTableHeader(rpt, pfo, HdLd[3+lf], HdRp[3+rf], HdDf[3+df]);
  printTableHeader(rpt, pfo, HdLd[4+lf], HdRp[4+rf], HdDf[4+df]);
  rpt->line += 5;
  return;
}

void printOvldHeader(cf_Out *rpt, Link *pfo)
/****************************************************************************\
* COULD USE DEFINE STATEMENTS 
\****************************************************************************/
{
  char *HdLd[] = { /* lead */
    ".|   |OVERLOAD CAUSED BY OUTAGE| | |  |  | |   |      CASE NAME: ->      | |  |  |    |   |",
    ".|   |                         | | |  |  | |   |                         | |  |  |    |   |",
    ".|   |   Overloaded Branches   | | |  |  | |   |    Outaged  Branches    | |  |  |CRTC|IN |",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|S|Z1|Z2|T|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|Z1|Z2|RATG|MYY|",
    " ",
    ".|   |OVERLOAD CAUSED BY OUTAGE| | |   |      CASE NAME: ->      | |    |",
    ".|   |                         | | |   |                         | |    |",
    ".|   |   Overloaded Branches   | | |   |    Outaged  Branches    | |CRTC|",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|T|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|RATG|",
    " ",
    ".|   |OVERLOAD CAUSED BY OUTAGE| | |  |  |    |   |",
    ".|   | Overload followed by... | | |  |  |    |   |",
    ".|   |      ...List of Outages | | |  |  |CRTC|IN |",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|S|Z1|Z2|RATG|MYY|",
    " ",
  };

  char *HdRp[] = { /* repeat */
    "%16.16s||||",
    "                ||||",
    " PRE  | POST |POST||",
    " LOAD | LOAD |(%)|F|",
    " ",
    "%4.4s||",
    "    ||",
    "POST||",
    "(%)|F|",
    " ",
  };

  char *HdDf[] = { /* difference */
    "", "", "", "", "",
    "       |       |",
    "POST(%)|POST(%)|",
    "VARIANC|VARIANC|",
    "   UP  |  DOWN |",
    " ",
  };

  int lf, rf, df;

  if (rpt->form==FORM_PAGE_WIDE) lf = 0;
  if (rpt->form==FORM_DATA_FULL) lf = 0;
  if (rpt->form==FORM_DATA_BREF) lf = 5;
  if (rpt->form==FORM_PAGE_COMP) lf = 10;
  rf = (rpt->form==FORM_DATA_BREF) ? 5 : 0;
  df = (rpt->diff==VARI_YES) ? 5 : 0;
  printTableCases(rpt,  pfo, HdLd[0+lf], HdRp[0+rf], HdDf[0+df]);
  printTableHeader(rpt, pfo, HdLd[1+lf], HdRp[1+rf], HdDf[1+df]);
  printTableHeader(rpt, pfo, HdLd[2+lf], HdRp[2+rf], HdDf[2+df]);
  printTableHeader(rpt, pfo, HdLd[3+lf], HdRp[3+rf], HdDf[3+df]);
  printTableHeader(rpt, pfo, HdLd[4+lf], HdRp[4+rf], HdDf[4+df]);
  rpt->line += 5;
  return;
}

void printBusvHeader(cf_Out *rpt, Link *pfo)
/****************************************************************************\
* COULD USE DEFINE STATEMENTS
\****************************************************************************/
{
  char *HdLd[] = { /*  lead  */
    ".|   |U/O VOLT DUE|  | |   |      CASE NAME: ->      | |  |  |     |     |",
    ".|   | TO  OUTAGE |  | |   |                         | |  |  |LOWER|UPPER|",
    ".|   |U/O  Voltage|  | |   |    Outaged  Branches    | |  |  |LIMIT|LIMIT|",
    ".|OWN|< BUS1 ><V1>|ZN|T|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|Z1|Z2|( PU)|( PU)|",
    " ",
    ".|   |U/O VOLT DUE|  | |   |      CASE NAME: ->      | |",
    ".|   | TO  OUTAGE |  | |   |                         | |",
    ".|   |U/O  Voltage|  | |   |    Outaged  Branches    | |",
    ".|OWN|< BUS1 ><V1>|ZN|T|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|",
    " ",
    ".|   |U/O VOLTAGE DUE TO OUTAGE| | |  |  |     |     |",
    ".|   | Bus followed by...      | | |  |  |LOWER|UPPER|",
    ".|   |      ...List of Outages | | |  |  |LIMIT|LIMIT|",
    ".|OWN|< BUS1 ><V1> < BUS2 ><V2>|C|S|Z1|Z2|( PU)|( PU)|",
    " ",
  };

  char *HdRp[] = { /* repeat */
    "%16.16s|||",
    "                |||",
    "ACTUA|ACTUA|CHANGE|",
    "( KV)|( PU)|( PU )|",
    " ",
    "%5.5s|",
    "     |",
    "ACTUA|",
    "( PU)|",
    " ",
  };

  char *HdDf[] = { /* difference */
    "", "", "", "", "",
    "       |       |",
    "VARIANC|VARIANC|",
    "  UP   | DOWN  |",
    "(  PU )|(  PU )|",
    " ",
  };

  int lf, rf, df;

  if (rpt->form==FORM_PAGE_WIDE) lf = 0;
  if (rpt->form==FORM_DATA_FULL) lf = 0;
  if (rpt->form==FORM_DATA_BREF) lf = 5;
  if (rpt->form==FORM_PAGE_COMP) lf = 10;
  rf = (rpt->form==FORM_DATA_BREF) ? 5 : 0;
  df = (rpt->diff==VARI_YES) ? 5 : 0;
  printTableCases(rpt,  pfo, HdLd[0+lf], HdRp[0+rf], HdDf[0+df]);
  printTableHeader(rpt, pfo, HdLd[1+lf], HdRp[1+rf], HdDf[1+df]);
  printTableHeader(rpt, pfo, HdLd[2+lf], HdRp[2+rf], HdDf[2+df]);
  printTableHeader(rpt, pfo, HdLd[3+lf], HdRp[3+rf], HdDf[3+df]);
  printTableHeader(rpt, pfo, HdLd[4+lf], HdRp[4+rf], HdDf[4+df]);
  rpt->line += 5;
  return;
}

void printTableCases(cf_Out *rpt, Link *curPfo, char *l, char *r, char *d)
{
  int width;

  fprintf(rpt->file, "%s", l);
  for (width=rpt->wide; curPfo!=NULL && width-- > 0; curPfo=curPfo->next) {
    fprintf(rpt->file, r, curPfo->data); /* case name */
  }
  fprintf(rpt->file, "%s", d);
  fprintf(rpt->file, "\n");
}


void printTableHeader(cf_Out *rpt, Link *curPfo, char *l, char *r, char *d)
{
  int width;

  fprintf(rpt->file, "%s", l); /* lead */
  for (width=rpt->wide; curPfo!=NULL && width-- > 0; curPfo=curPfo->next) {
    fprintf(rpt->file, "%s", r); /* repeat */
  }
  fprintf(rpt->file, "%s", d); /* difference */
  fprintf(rpt->file, "\n");
}


void printOutg(FILE *fp, Link *outgLink, int form)
{
  char id[CF_STRSIZE], out[CF_STRSIZE];
  pf_rec d;
  cf_Branch *outgData;
  cf_Name *comoData;

  outgData = (outgLink!=NULL) ? (cf_Branch *) outgLink->data : NULL;
  if (outgData==NULL || outgLink==NULL) {
     if      (form==FORM_PAGE_COMP) sprintf(out, "%41.41s|", "");
    else if (form==FORM_DATA_BREF) sprintf(out, "%32.32s|", "");
    else                           sprintf(out, "%39.39s|", "");
  }
  else if (outgData->type[0]!='c') {
    cf_branch2rec(outgData, &d);
    pf_rec_b2a(id, &d, "I");
    if (form==FORM_PAGE_COMP)
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s %-12.12s|%1.1s| |%-2.2s|%-2.2s|",
            &id[0], &id[3], &id[6], &id[19], &id[31], outgData->bus1_zone,
            outgData->bus2_zone);
    else if (form==FORM_DATA_BREF)
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s %-12.12s|%1.1s|",
            &id[0], &id[3], &id[6], &id[19], &id[31]);
    else
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s %-12.12s|%1.1s|%-2.2s|%-2.2s|",
            &id[0], &id[3], &id[6], &id[19], &id[31], outgData->bus1_zone,
            outgData->bus2_zone);
  }
  else if (outgData->type[0]=='c') { /* common mode */
    comoData = (cf_Name *) outgData;

/************* logic added for missing elements in common mode outage **/
    if ( comoData->me_flag == 1)
       {
       if      (form==FORM_PAGE_COMP) sprintf(out,"%-35.35s|||#E#||||",comoData->name);
       else if (form==FORM_DATA_BREF) sprintf(out,"%-30.30s|#E#|||",   comoData->name);
       else                           sprintf(out,"%-34.34s|||#E#|||", comoData->name);

       }
    else
       {
       if      (form==FORM_PAGE_COMP) sprintf(out,"%-35.35s|||||||",comoData->name);
       else if (form==FORM_DATA_BREF) sprintf(out,"%-30.30s||||",   comoData->name);
       else                           sprintf(out,"%-34.34s||||||", comoData->name);
       }
/************* logic added for missing elements in common mode outage **/

  }
  fprintf(fp, "%s", out);
}

void printOvld(FILE *fp, Link *ovldLink, int form)
{
  char id[CF_STRSIZE], out[CF_STRSIZE];
  pf_rec d;
  cf_Branch *ovldData;

  ovldData = (ovldLink!=NULL) ? (cf_Branch *) ovldLink->data : NULL;
  if (ovldData==NULL || ovldLink==NULL) {
    if (form==FORM_DATA_BREF) sprintf(out, "%32.32s|", "");
    else                      sprintf(out, "%41.41s|", "");
  }
  else if (ovldData->type[0]!='s') {
    cf_branch2rec(ovldData, &d);
    pf_rec_b2a(id, &d, "I");
    if (form==FORM_DATA_BREF)
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s %-12.12s|%1.1s|",
            &id[0], &id[3], &id[6], &id[19], &id[31]);
    else
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s %-12.12s|%1.1s|%1.1s|%-2.2s|%-2.2s|",
            &id[0], &id[3], &id[6], &id[19], &id[31], &id[32],
            ovldData->bus1_zone, ovldData->bus2_zone);
  }
  else if (ovldData->type[0]=='s') {
    if (form==FORM_DATA_BREF)
        sprintf(out, "      SOLUTION     PROBLEMS   ||||");
    else
        sprintf(out, "      SOLUTION     PROBLEMS        |||||||");
  }
  fprintf(fp, "%s", out);
}

void printBusv(FILE *fp, Link *busvLink, int form)
{
  char id[CF_STRSIZE], out[CF_STRSIZE];
  pf_rec d;
  cf_Bus *busvData;

  busvData = (busvLink!=NULL) ? (cf_Bus *) busvLink->data : NULL;
  if (busvData==NULL || busvLink==NULL) {
    if (form==FORM_DATA_BREF) sprintf(out, "%21.21s|", "");
    else                      sprintf(out, "%21.21s|", "");
  }
  else if (busvData->type[0]=='s') {
    if (form==FORM_PAGE_COMP) /* compact sparse */
        sprintf(out, "        SOLUTION     PROBLEMS    |||||||");
    else if (form==FORM_DATA_BREF)
        sprintf(out," SOLUTION PROBLEMS||||");
    else
        sprintf(out," SOLUTION PROBLEMS||||");
  }
  else if (busvData->type[0]!='s') {
    memset(&d, '\0', sizeof(pf_rec));
    memcpy(&d, busvData, sizeof(cf_Bus));
    pf_rec_b2a(id, &d, "I");
    if (form==FORM_PAGE_COMP) /* compact sparse */
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s %-12.12s|%1.1s| |%-2.2s|%-2.2s|",
            &id[0], &id[3], &id[6], "", "", &id[18], "");
    else if (form==FORM_DATA_BREF)
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s|%-2.2s|",
            &id[0], &id[3], &id[6], &id[18]);
    else
        sprintf(out, "%-1.1s|%-3.3s|%-12.12s|%-2.2s|",
            &id[0], &id[3], &id[6], &id[18]);
  }
  fprintf(fp, "%s", out);
}

void printDiff(cf_Out *rpt, Link *solnLink, int mode)
{
  ff_soln *varData;

  while (solnLink!=NULL && solnLink->next!=NULL) solnLink = solnLink->next;
  if (mode==0 || solnLink==NULL || solnLink->data==NULL) {
    fprintf(rpt->file, "       |       |");
  }
  else {
    varData = (ff_soln *) solnLink->data;
    if (mode==1) {
        fprintf(rpt->file, "%7.1f|%7.1f|", varData->max_new, varData->min_ref);
    }
    if (mode==2) {
        fprintf(rpt->file, "%7.3f|%7.3f|", varData->max_new, varData->min_ref);
    }
  }
  return;
}

void printLoad(cf_Out *rpt, Link *solnLink, int doN)
/***************************************************************************\
*
\***************************************************************************/
{
  Link    *curLink, *refLink;
  ff_soln *curSoln, *refSoln;
  char     code;

  refLink=solnLink;                /* find first valid data for CRITCL RATING */
  for (; refLink!=NULL && refLink->data==NULL; refLink=refLink->next);
  if (refLink!=NULL && refLink->data!=NULL) {
    refSoln = (ff_soln *) refLink->data;
    if (rpt->form==FORM_DATA_BREF)
        fprintf(rpt->file, "%4.0f|", refSoln->max_new);
    else
        fprintf(rpt->file, "%4.0f|%3.3s|",refSoln->max_new,refSoln->date);
  }
  else 
    fprintf(rpt->file, "    |   |");

  for (curLink = solnLink; curLink!=NULL; curLink=curLink->next) {
    if (doN-- <= 0) break;
    if (curLink->data==NULL) {
        if (rpt->form==FORM_DATA_BREF) fprintf(rpt->file, "   | |");
        else fprintf(rpt->file, "      |      |   | |");
    }
    else {
        curSoln = (ff_soln *) curLink->data;
        code = curSoln->code;
        if (refLink->data!=NULL && curSoln->max_new != refSoln->max_new) {
            code = tolower(curSoln->code);
        }
        if (rpt->form==FORM_DATA_BREF)
            fprintf(rpt->file, "%3.0f|%1.1s|", curSoln->ratio, &code);
        else {
            if ((int)curSoln->pre_con == -9999) /* no pre_con data found */
                fprintf(rpt->file, "      |%6.1f|%3.0f|%1.1s|",
                    curSoln->actual, curSoln->ratio, &code);
            else
                fprintf(rpt->file, "%6.1f|%6.1f|%3.0f|%1.1s|",
                    curSoln->pre_con, curSoln->actual, curSoln->ratio, &code);

        }
    }
  }

  while (doN-- > 0) {
    if (rpt->form==FORM_DATA_BREF) fprintf(rpt->file, "   | |");
    else fprintf(rpt->file, "      |      |   | |");
  }

  if (rpt->diff==VARI_YES) printDiff(rpt, solnLink, 1);
  fprintf(rpt->file, "\n");
  rpt->line += 1;

  return;
}

void printVolt(cf_Out *rpt, Link *solnLink, int doN)
/***************************************************************************\
*
\***************************************************************************/
{
  Link    *curLink, *refLink;
  ff_soln *curSoln, *refSoln;

  refLink=solnLink;                                            /* find LIMITS */
  for (; refLink!=NULL && refLink->data==NULL; refLink=refLink->next);
  if (refLink!=NULL && refLink->data!=NULL) {
    refSoln = (ff_soln *) refLink->data;
    if (rpt->form==FORM_DATA_BREF)
        fprintf(rpt->file, "");
    else
        fprintf(rpt->file, "%5.3f|%5.3f|",refSoln->min_ref, refSoln->max_new);
  }
  else {
    if (rpt->form==FORM_DATA_BREF)
        fprintf(rpt->file, "");
    else
        fprintf(rpt->file, "     |     |      |");
  }

  for (curLink = solnLink; curLink!=NULL; curLink=curLink->next) {
    if (doN-- <= 0) break;
    if (curLink->data==NULL || !*((int *)(curLink->data))) {
        if (rpt->form==FORM_DATA_BREF)
            fprintf(rpt->file, "     |");
        else
            fprintf(rpt->file, "     |     |      |");
    }
    else {
        curSoln = (ff_soln *) curLink->data;
        if (rpt->form==FORM_DATA_BREF)
            fprintf(rpt->file, "%5.3f|", curSoln->ratio);
        else {
            if ((int)curSoln->pre_con == -9999) /* no change data found */
                fprintf(rpt->file,"%5.1f|%5.3f|      |", curSoln->actual,
                    curSoln->ratio);
            else
                fprintf(rpt->file,"%5.1f|%5.3f|%6.3f|", curSoln->actual,
                    curSoln->ratio, curSoln->pre_con);
        }
    }
  }

  while (doN-- > 0) {
    if (rpt->form==FORM_DATA_BREF) fprintf(rpt->file, "     |");
    else fprintf(rpt->file, "     |     |      |");
  }

  if (rpt->diff==VARI_YES) printDiff(rpt, solnLink, 2);
  fprintf(rpt->file, "\n");
  rpt->line += 1;

  return;
}

void printProb(cf_Out *rpt, Link *probLink, int doN)
/***************************************************************************\
*
\***************************************************************************/
{
  char *Problem[] = {
    "    |   |",            /* overload report; blank rating, blank in-date */
    "                ||||", /* overload report; blank solution problem */
    "     %11.11s||||",     /* overload report; format for probData->name */
    "     |     |",         /* bus violation; blank lower-, upper-limit */
    "             ||",      /* bus violation; blank solution problem */
    "      %11.11s||",      /* bus violation; format for probData->name */
    "    |",                /* overload report; blank rating, blank code */
    "    ||",               /* overload report; blank solution problem */
    "%4.4s||",              /* overload report; format for probData->name */
    "",                     /* bus violation; blank lower-, upper-limit */
    "     |",               /* bus violation; blank solution problem */
    "%5.5s|",               /* bus violation; format for probData->name */
  };

  int      offset;
  Link    *curProb;
  ff_soln *probData;
  char     descrip[11];

  offset=(rpt->type==TYPE_OUTG_OVLD || rpt->type==TYPE_OVLD_OUTG) ? 0:3;
  offset += (rpt->form==FORM_DATA_BREF) ? 6 : 0;
  fprintf(rpt->file, Problem[0+offset]);

  for (curProb = probLink; curProb!=NULL; curProb=curProb->next) {
    if (doN-- <= 0) break;
    probData = (ff_soln *) curProb->data;
    if (probData==NULL)
        fprintf(rpt->file, Problem[1+offset]);
    else {
        if (rpt->form!=FORM_DATA_BREF) {
                 if (probData->code=='F') strcpy(descrip, "NO SOLUTION");
            else if (probData->code=='X') strcpy(descrip, "FAILED RX  ");
            else if (probData->code=='S') strcpy(descrip, "SEPARATION ");
            else if (probData->code=='K') strcpy(descrip, "SOLUTION OK");
            else                          strcpy(descrip, "UNKNOWN    ");
        }
        else {
                 if (probData->code=='F') strcpy(descrip, "FAIL");
            else if (probData->code=='X') strcpy(descrip, " RX ");
            else if (probData->code=='S') strcpy(descrip, "SEPA");
            else if (probData->code=='K') strcpy(descrip, " OK ");
            else                          strcpy(descrip, " ?? ");
        }
        fprintf(rpt->file, Problem[2+offset], descrip);
    }
  }

  while (doN-- > 0) fprintf(rpt->file, Problem[1+offset]);
  if (rpt->diff==VARI_YES) printDiff(rpt, probLink, 0);
  fprintf(rpt->file, "\n");
  rpt->line += 1;

  return;
}

int validSoln(Link *solnLink, Trace *trace )
{
 ff_soln *solnData;

 while (solnLink!=NULL) {
    solnData = (ff_soln *) solnLink->data;
    if (solnData!=NULL) {
        if ( trace->listSeparations==1 || solnData->code != 'S' )
            return 1;  /* keep if solution failed for other than a separation */
    }
    solnLink = solnLink->next;
 }                                 

 return 0;
}


int validInput(Link *listPtr, cf_Branch *data)
{ /* uses a start-at-beginning-and-search-whole-list approach */
  cf_Branch *listData;
  int        cmp;

  if (listPtr==NULL) return -1;
  for (cmp=1; listPtr!=NULL; listPtr=listPtr->next) {
    listData = (cf_Branch *) listPtr->data;
    if (listData->type[0] != data->type[0]) continue;
    if (data->type[0]=='c') {                          /* common-mode outages */
        cmp = ff_cmpName((cf_Name *) listData, (cf_Name *) data);
    }
    else if (data->type[0]=='B') {                                   /* buses */
        cmp = cf_cmpBus( (pf_AC_bus *) listData, (pf_AC_bus *) data );
    }
    else {                                    /* branch outages and overloads */
        cmp = ff_wildBranch( (pf_branch *) listData, (pf_branch *) data );
    }
    if (cmp==0) return 1;
  }
  return 0;
}

int validVariance(Trace trace, Link *refLink)
/****************************************************************************\
* return 1 if difference with problem solving, or variation exceeds span 
\****************************************************************************/
{
  Link       *variLink, *solnLink;
  ff_soln    *refData, *solnData, *variData;
  float       refRat, solRat, solMax, solMin, varUp, varDown, varSpan;

/*  int         refCode, solCode;           original code */
  char         refCode, solCode;

       /* get data from 1st .PFO file */
  refData = (refLink==NULL) ? NULL : (ff_soln *) refLink->data;
  refCode = (refData==NULL) ? ' ' : ((ff_soln *) refLink->data)->code;

/********* if the REFERENCE case failed or did not contain the outage, report it ***/
  if ((refCode==' ')||(refCode=='S')||(refCode=='X')||(refCode=='F')) return 1;
 
  for (solnLink=refLink->next; solnLink!=NULL; solnLink=solnLink->next) {
    solCode = (solnLink->data!=NULL) ? ((ff_soln *)solnLink->data)->code : ' ';

/*    if ((refCode=='S')|(refCode=='X')|(refCode=='F')|(refCode=='K')) {  original code */
/*        if (refCode!=solCode) return 1;                                 original code */
/*    }                                                                   original code */

/********* if a SUBSEQUENT case failed or did not contain the outage, report it ***/
      if ((solCode==' ')||(solCode=='S')||(solCode=='X')||(solCode=='F')) return 1;

  }

/**************************************************************************/
  refRat = (refData==NULL) ? 0 : refData->ratio;
  solMax = refRat;
  solMin = refRat;

	/* loop to find largest rate */
  for (solnLink=refLink->next; solnLink!=NULL; solnLink=solnLink->next) {
    solnData = (ff_soln *)solnLink->data;
    solRat = (solnData==NULL) ? 0 : solnData->ratio;
    solMax = Cf_max(solMax, solRat);
    solMin = Cf_min(solMin, solRat);
  }
  varUp   = solMax - refRat;            /* i.e.   3% =  101% - 98% */
  varDown = solMin - refRat;            /* i.e.  -5% =   93% - 98% */
  varSpan = varUp - varDown;

  if (refData==NULL) {
    varUp = varDown = varSpan = 0;
  }

  variLink = cf_newLink(sizeof(ff_soln));
  cf_appList(&refLink, variLink);
  variData = (ff_soln *) variLink->data;

  variData->min_ref = varDown;
  variData->max_new = varUp;
  variData->actual  = varSpan;
  variData->ratio   = (varUp > -1*varDown) ? varUp : varDown;
  variData->code    = ' ';

  if (varUp<trace.varCeiling && varDown>trace.varFloor) return 0;

  return 1;
}

int validLoad(Trace trace, Link *solnLink)
{
  ff_soln *solnData;

  while (solnLink!=NULL) {
    solnData = (ff_soln *) solnLink->data;
    if (solnData!=NULL && solnData->ratio >= trace.minLoadPct) return 1;
    solnLink = solnLink->next;
  }

  return 0;
}


int validVolt(Trace trace, Link *solnLink)
{
  ff_soln *solnData;
  float vpu, basekv;
  cf_Bus  *busvData;
  if (solnLink==NULL || solnLink->prev==NULL || solnLink->prev->data==NULL)
    return 0;
  busvData = (cf_Bus *) solnLink->prev->data;
  basekv = busvData->kv;
  for (; solnLink!=NULL; solnLink = solnLink->next) {
    if ( (solnData = (ff_soln *) solnLink->data) == NULL ) continue;
    vpu = solnData->ratio;
    if      ((int)basekv==500 && (vpu>=trace.max5 || vpu<=trace.min5)) return 1;
    else if ((int)basekv!=500 && (vpu>=trace.maxV || vpu<=trace.minV)) return 1;
  }
  return 0;
}

int ff_wildBranch(pf_branch *b1, pf_branch *b2)
/******************************************************************************\
* ff_wildBranch: compare id data of two pf_branch records with wild card.
* pf_branch *b1 - ptr to branch rec. with optional wild card ckt_id and section.
* pf_branch *b2 - pointer to branch record.
* returns: int - 0 if same, 1 if b1 > b2, -1 if b1 < b2.
* developed by: WDRogers  created: 08-22-96
\******************************************************************************/
{
  float c;

  if ( c=strncmp(b1->type, b2->type, 1) )       return c>0 ? 1 : -1;
  if ( c=strcmp(b1->bus1_name, b2->bus1_name) ) return c>0 ? 1 : -1;
  if ( fabs(c=b1->bus1_kv-b2->bus1_kv) > .001 ) return c>0 ? 1 : -1;
  if ( c=strcmp(b1->bus2_name, b2->bus2_name) ) return c>0 ? 1 : -1;
  if ( fabs(c=b1->bus2_kv-b2->bus2_kv) > .001 ) return c>0 ? 1 : -1;
  if ( b1->ckt_id != ' ' ) {       /* if blank, accept any ckt_id */
    if ( c=b1->ckt_id - b2->ckt_id )            return c>0 ? 1 : -1;
  }
  if ( b1->section != 0 ) {        /* if zero, accept any section */
    if ( c=b1->section - b2->section )          return c>0 ? 1 : -1;
  }
  return 0;
}

/* hash:  form hash value for string s */
unsigned hash(char *s)
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}


/* lookup:  look for s in hashtab */
struct nlist *lookup(char *s)
{
    struct nlist *np;

    for (np = hashtab[hash(s)]; np != NULL; np = np->next)
        if (strcmp(s, np->name) == 0) {
          if (strstr (s, "B/D **MAP VAL SECT 1 (NO BOTH LINE)") != NULL) {
             printf ("lookup: np = %d s = %s\n", np, s);
          }
          return np;  /* found */
        }
    return NULL;        /* not found */
}

/* install: put (name, keep) in hash tab */

struct nlist *install(char *name, int keep)
{
    struct nlist *np;
    unsigned hashval;

    if (strstr (name, "B/D **MAP VAL SECT 1 (NO BOTH LINE)") != NULL) {
      printf ("install: %s\n", name);
    }
    if ((np = lookup(name)) == NULL) { /* not found */
        np = (struct nlist *) malloc(sizeof(*np));
        if (np == NULL || (np->name = cf_strdup(name)) == NULL)
            return NULL;
        hashval = hash(name);
        np->next = hashtab[hashval];
        hashtab[hashval] = np;
    }
    np->keep = keep;
    return np;
}    

void ff_stream2List(FILE *readMe, Trace *trace, Link **expList)
/***************************************************************************\
*
\***************************************************************************/
{
  char str[CF_INBUFSIZE], STR[CF_INBUFSIZE];
  Link *list;
  int mode = READ_INC; /* default /INCLUDE */

  while (fgets(str, CF_INBUFSIZE, readMe)!=NULL) { 
    if ( cf_iscomment(str) ) continue;
    strcpy(STR, str);
    cf_str2upper(STR);
    if ( strstr(STR, "/INCLUDE"     )!=NULL ) { mode = READ_INC; continue; }
    if ( strstr(STR, "/PFO"         )!=NULL ) { mode = READ_PFO; continue; }
    if ( strstr(STR, "/OUTAGE"      )!=NULL ) { mode = READ_OUT; continue; }
    if ( strstr(STR, "/COMMON_MODE" )!=NULL ) { mode = READ_COM; continue; }
    if ( strstr(STR, "/OVERLOAD"    )!=NULL ) { mode = READ_OVR; continue; }
    if ( strstr(STR, "/BUS"         )!=NULL ) { mode = READ_BUS; continue; }
    if ( strstr(STR, "/OUTG_OWNER"  )!=NULL ) { mode = READ_OWN; continue; }
    if ( strstr(STR, "/OUTG_ZONE"   )!=NULL ) { mode = READ_ZON; continue; }
    if ( strstr(STR, "/OUTG_BASE_KV")!=NULL ) { mode = READ_BKV; continue; }
    if ( strstr(STR, "/PROB_OWNER"  )!=NULL ) { mode = READ_OWP; continue; }
    if ( strstr(STR, "/PROB_ZONE"   )!=NULL ) { mode = READ_ZNP; continue; }
    if ( strstr(STR, "/PROB_BASE_KV")!=NULL ) { mode = READ_KVP; continue; }
    if ( strstr(STR, "/LIMITS"      )!=NULL ) { mode = READ_LIM; continue; }
    if ( strstr(STR, "/REPORT"      )!=NULL ) { mode = READ_REP; continue; }
    if ( STR[0]=='/' ) { mode = 0; continue; }     /* unrecognized slash card */
    if ( mode == 0 ) continue;         /* note: mode is defaulted to READ_INC */
    if ( mode == READ_INC ) {
        list = cf_text2List(str);
        ff_expList(trace, list, expList);
    }

    if ( mode == READ_PFO ) {
        list = cf_text2List(str);
        cf_appList(&trace->pfoList, list);
        if (list!=NULL) trace->query &= ~(QUERY_PFO);
    }

    if ( mode == READ_OUT ) {
        if (strchr(STR, FF_WILD)==NULL) {
            list = cf_id2Link(STR, 'L');
            cf_appList(&trace->outgList, list);
        }
        else {
            list = cf_text2Link(STR);
            cf_appList(&trace->outgMask, list);
        }
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OUTG);
        if (list!=NULL) trace->query &= ~(QUERY_COMO);
        if (list!=NULL) trace->query &= ~(QUERY_ZONE);
        if (list!=NULL) trace->query &= ~(QUERY_OWNR);
        if (list!=NULL) trace->query &= ~(QUERY_BSKV);
    }

    if ( mode == READ_COM ) {
        if (strchr(STR, FF_WILD)==NULL) {
            list = getCoMoOutg(STR);
            cf_appList(&trace->comoList, list);
        }
        else {
            list = cf_text2Link(STR);
            cf_appList(&trace->comoMask, list);
        }
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OUTG);
        if (list!=NULL) trace->query &= ~(QUERY_COMO);
        if (list!=NULL) trace->query &= ~(QUERY_ZONE);
        if (list!=NULL) trace->query &= ~(QUERY_OWNR);
        if (list!=NULL) trace->query &= ~(QUERY_BSKV);
    }

    if ( mode == READ_OVR ) {
        if (strchr(STR, FF_WILD)==NULL) {
            list = cf_id2Link(STR, 'L');
            cf_appList(&trace->ovldList, list);
        }
        else {
            list = cf_text2Link(STR);
            cf_appList(&trace->ovldMask, list);
        }
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OVLD);
        if (list!=NULL) trace->query &= ~(QUERY_BUSV);
        if (list!=NULL) trace->query &= ~(QUERY_ZN_P);
        if (list!=NULL) trace->query &= ~(QUERY_OW_P);
        if (list!=NULL) trace->query &= ~(QUERY_KV_P);
    }

    if ( mode == READ_BUS ) {
        if (strchr(STR, FF_WILD)==NULL) {
            list = cf_id2Link(STR, 'I');
            cf_appList(&trace->busvList, list);
        }
        else {
            list = cf_text2Link(STR);
            cf_appList(&trace->busvMask, list);
        }
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OVLD);
        if (list!=NULL) trace->query &= ~(QUERY_BUSV);
        if (list!=NULL) trace->query &= ~(QUERY_ZN_P);
        if (list!=NULL) trace->query &= ~(QUERY_OW_P);
        if (list!=NULL) trace->query &= ~(QUERY_KV_P);
    }
    if ( mode == READ_OWN ) {
        list = cf_text2List(STR);
        if (list!=NULL) trace->query &= ~(QUERY_OWNR);
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OUTG);
        if (list!=NULL) trace->query &= ~(QUERY_COMO);
        cf_appList(&trace->ownrOutg, list);
    }
    if ( mode == READ_ZON ) {
        list = cf_text2List(STR);
        if (list!=NULL) trace->query &= ~(QUERY_ZONE);
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OUTG);
        if (list!=NULL) trace->query &= ~(QUERY_COMO);
        cf_appList(&trace->zoneOutg, list);
    }
    if ( mode == READ_BKV ) {
        list = cf_text2List(STR);
        if (list!=NULL) trace->query &= ~(QUERY_BSKV);
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OUTG);
        if (list!=NULL) trace->query &= ~(QUERY_COMO);
        cf_appList(&trace->bskvOutg, list);
    }
    if ( mode == READ_OWP ) {
        list = cf_text2List(STR);
        if (list!=NULL) trace->query &= ~(QUERY_OW_P);
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OVLD);
        if (list!=NULL) trace->query &= ~(QUERY_BUSV);
        cf_appList(&trace->ownrProb, list);
    }
    if ( mode == READ_ZNP ) {
        list = cf_text2List(STR);
        if (list!=NULL) trace->query &= ~(QUERY_ZN_P);
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OVLD);
        if (list!=NULL) trace->query &= ~(QUERY_BUSV);
        cf_appList(&trace->zoneProb, list);
    }
    if ( mode == READ_KVP ) {
        list = cf_text2List(STR);
        if (list!=NULL) trace->query &= ~(QUERY_KV_P);
        if (list!=NULL) trace->query &= ~(QUERY_DATA);
        if (list!=NULL) trace->query &= ~(QUERY_OVLD);
        if (list!=NULL) trace->query &= ~(QUERY_BUSV);
        cf_appList(&trace->bskvProb, list);
    }
    if ( mode == READ_LIM ) ff_limits(STR, trace);
    if ( mode == READ_REP ) ff_report(str, trace);
  }
  return;
}

void ff_limits(char *s, Trace *trace)
{
  float f;
  if (strstr(s, "VARIANCE_CEILING")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->varCeiling = f;
    trace->query &= ~(QUERY_SPAN);
  }
  if (strstr(s, "VARIANCE_FLOOR")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->varFloor = f;
    trace->query &= ~(QUERY_SPAN);
  }
  if (strstr(s, "MIN_LOAD_PCT")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->minLoadPct = f;
    trace->query &= ~(QUERY_LOAD);
  }
  if (strstr(s, "MAX_500_PU")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->max5 = f;
    trace->query &= ~(QUERY_VOLT);
  }
  if (strstr(s, "MIN_500_PU")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->min5 = f;
    trace->query &= ~(QUERY_VOLT);
  }
  if (strstr(s, "MAX_BUS_PU")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->maxV = f;
    trace->query &= ~(QUERY_VOLT);
  }
  if (strstr(s, "MIN_BUS_PU")!=NULL) {
    sscanf(s, "%*s = %f", &f);
    trace->minV = f;
    trace->query &= ~(QUERY_VOLT);
  }
  if (strstr(s, "INC_SOLN_PROB")!=NULL) {
    if (strstr(s, "NO" )!=NULL) trace->soln = SKP_SOLN_PROB;
    if (strstr(s, "YES")!=NULL) trace->soln = INC_SOLN_PROB;
    trace->query &= ~(QUERY_SOLN);
  }
}

void ff_report(char *s, Trace *trace)
{
  if (strstr(s, "NAME"           )!=NULL) {
    sscanf(s, "%*s = %s", trace->outName);
    trace->query &= ~(QUERY_NAME);
    return;
  }
  if (strstr(s, "TYPE"           )!=NULL) {
    if (strstr(s, "OUTAGE-OVERLOAD" )!=NULL) trace->type = TYPE_OUTG_OVLD;
    if (strstr(s, "OVERLOAD-OUTAGE" )!=NULL) trace->type = TYPE_OVLD_OUTG;
    if (strstr(s, "OVERLOAD-BOTH"   )!=NULL) trace->type = TYPE_OVLD_BOTH;
    if (strstr(s, "OUTAGE-BUS_V"    )!=NULL) trace->type = TYPE_OUTG_BUSV;
    if (strstr(s, "BUS_V-OUTAGE"    )!=NULL) trace->type = TYPE_BUSV_OUTG;
    if (strstr(s, "BUS_V-BOTH"      )!=NULL) trace->type = TYPE_BUSV_BOTH;
    if (trace->type==TYPE_OUTG_OVLD) trace->query &= ~(QUERY_BUSV | QUERY_VOLT);
    if (trace->type==TYPE_OVLD_OUTG) trace->query &= ~(QUERY_BUSV | QUERY_VOLT);
    if (trace->type==TYPE_OVLD_BOTH) trace->query &= ~(QUERY_BUSV | QUERY_VOLT);
    if (trace->type==TYPE_OUTG_BUSV) trace->query &= ~(QUERY_OVLD | QUERY_LOAD);
    if (trace->type==TYPE_BUSV_OUTG) trace->query &= ~(QUERY_OVLD | QUERY_LOAD);
    if (trace->type==TYPE_BUSV_BOTH) trace->query &= ~(QUERY_OVLD | QUERY_LOAD);
    trace->query &= ~(QUERY_TYPE);
    return;
  }
  if (strstr(s, "CASES_PER_TABLE")!=NULL) {
    sscanf(s, "%*s = %d", &trace->trc->wide);
    if (trace->trc->wide<=0) trace->trc->wide = 1;
    trace->query &= ~(QUERY_WDTH);
  }
  if (strstr(s, "FORMAT"    )!=NULL) {
    if (strstr(s, "DATA" )!=NULL) trace->trc->form = FORM_DATA_FULL; /* default  */
    if (strstr(s, "BRIEF")!=NULL) trace->trc->form = FORM_DATA_BREF; /* modifier */
    if (strstr(s, "PAGE" )!=NULL) trace->trc->form = FORM_PAGE_WIDE; /* default  */
    if (strstr(s, "COMP" )!=NULL) trace->trc->form = FORM_PAGE_COMP; /* modifier */
    trace->query &= ~(QUERY_FORM);
  }
  if (strstr(s, "SORT"  )!=NULL) {
    if (strstr(s, "ALPHA"      )!=NULL) trace->sort = SORT_BY_ALPHA;
    if (strstr(s, "BASE_KV"    )!=NULL) trace->sort = SORT_BY_BASEKV;
    if (strstr(s, "OWNER"      )!=NULL) trace->sort = SORT_BY_OWNER;
    if (strstr(s, "ZONE"       )!=NULL) trace->sort = SORT_BY_ZONE;
    if (strstr(s, "INPUT"      )!=NULL) trace->sort = SORT_BY_INPUT;
    if (strstr(s, "SOLUTION"   )!=NULL) trace->sort = SORT_BY_SOLN;
    if (strstr(s, "DIFFERENCE" )!=NULL) trace->sort = SORT_BY_SOLN;
    trace->query &= ~(QUERY_SORT);
  }
  if (strstr(s, "TRACE"   )!=NULL) {
    if (strstr(s, "YES")!=NULL) trace->trc->type = TYPE_TRACE;
    if (strstr(s, "NO" )!=NULL) trace->trc->type = TYPE_NONE;
    trace->query &= ~(QUERY__TRC);
  }
  if (strstr(s, "VARIANCE"   )!=NULL) {
    if (strstr(s, "YES")!=NULL) trace->trc->diff = VARI_YES;
    if (strstr(s, "NO" )!=NULL) trace->trc->diff = VARI_NO;
    trace->query &= ~(QUERY_VARI);
  }
  if (strstr(s, "REMOVE_REDUNDANT")!=NULL) {
    if (strstr(s, "YES")!=NULL) trace->redun = REDUN_REMOVE;
    if (strstr(s, "NO" )!=NULL) trace->redun = REDUN_KEEP;
    trace->query &= ~(QUERY_REDU);
  }

  if (strstr(s, "LIST_SEPARATIONS")!=NULL) {
    if (strstr(s, "YES")!=NULL) trace->listSeparations = YES;
    if (strstr(s, "NO" )!=NULL) trace->listSeparations = 0;
    trace->query &= ~(QUERY_SSEP);
  }
}

void ff_expList(Trace *trace, Link *dataList, Link **expList)
{ /* expand double-linked list */
  Link *curLink, *newList;
  FILE *readMe;

  curLink = dataList;
  while (curLink != NULL) {
    if ( cf_isDataFile(curLink->data) ){/*DAT, LIS, or TRC to be opened & read*/
        readMe = cf_openFile(curLink->data, "r");/* it's okay if readMe==NULL */
        ff_stream2List(readMe, trace, expList);
        fclose(readMe);
    }
    else if (expList!=NULL) { /* expansion list */
        newList = cf_text2List(curLink->data);
        cf_appList(expList, newList);
    }
    curLink = cf_delLink(&dataList, curLink);
  }
}


int ff_srtBranch(cf_Branch *b1, cf_Branch *b2, int sort) /* sort branch */
{
  int c;
  float d;
  if ( (c=strcmp(b1->type, b2->type)) != 0 ) return c;         /* [type]-sort */
  if (sort==SORT_BY_ZONE) {                                /* [zone]-alpha-kv */
    if ( (c=strcmp(b1->bus1_zone, b2->bus1_zone)) != 0 ) return c;
    if ( (c=strcmp(b1->bus2_zone, b2->bus2_zone)) != 0 ) return c;
  }
  else if (sort==SORT_BY_OWNER) {                         /* [owner]-alpha-kv */
    if ( (c=strcmp(b1->owner, b2->owner)) != 0 ) return c;
  }
  else if (sort==SORT_BY_BASEKV) {                           /* [kv]-alpha-kv */
    if ( fabs(d = b1->bus1_kv - b2->bus1_kv) > .001 ) return d;
    if ( fabs(d = b1->bus2_kv - b2->bus2_kv) > .001 ) return d;
  }
  return cf_cmpBranch((pf_branch *) b1, (pf_branch *) b2);         /* [alpha-kv] */
}

int validMask(Link *maskList, cf_Name *r)
{
  Link *maskLink;
  char *maskData, netData[CF_STRSIZE], netVal[34], mskVal[34];
  int   n;
  pf_rec t;

  if (maskList==NULL || maskList->data==NULL) return -1;
  for (maskLink=maskList; maskLink!=NULL; maskLink=maskLink->next) {
    maskData = (char *) maskLink->data;
    switch (toupper(r->type[0])) {
        case 'B' :  cf_bus2rec((cf_Bus *) r, &t); 
                    pf_rec_b2a(netData, &t, "I"); break;
        case 'E' :
        case 'L' :
        case 'T' :  cf_branch2rec((cf_Branch *) r, &t); 
                    pf_rec_b2a(netData, &t, "I"); break;
        case 'C' :  strcpy(netData, r->name); break;
        default  :  return 0;                      /* data at *r is not valid */
    }
    sprintf(mskVal, "%-33.33s", maskData);
    sprintf(netVal, "%-33.33s", netData);
    for (n=0; mskVal[n]!='\0'; n++) {
        if (mskVal[n] == '?') netVal[n] = '?';
        if (mskVal[n] == '*') netVal[n] = '*';
        if (mskVal[n] == ' ') netVal[n] = ' ';
        if (mskVal[n] == '0') netVal[n] = '0';
    }
    if (strncmp(mskVal, netVal, 33)==0) return 1;                     /* keep */
  }
  return 0;
}
/************ documented, common CF_UTIL.H candidates prototypes **************/
/* all documented, CF_UTIL.H candidates shall be designated, cf_name().       */
void date(char *date)
{
  int mo, yr;
  if (sscanf(date, "%d/%d", &mo, &yr) != 2) return;
  if (mo>12 || mo<1 || yr<0 || yr>99) return;
  if      (mo==10) sprintf(date, "O%02d", yr);
  else if (mo==11) sprintf(date, "N%02d", yr);
  else if (mo==12) sprintf(date, "D%02d", yr);
  else             sprintf(date, "%1d%02d", mo, yr);
}
/******** end documented, common CF_UTIL.H candidates prototypes **************/
