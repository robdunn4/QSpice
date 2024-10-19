/*******************************************************************************
 * CBlockBasics8_pp.cpp -- Example stand-alone post-processing program for
 * C-Block Basics #8 project.
 *
 * See https://github.com/robdunn4/QSpice/ for additional documentation/code.
 ******************************************************************************/
//
// To build with Digital Mars C++ Compiler:
//    dmc -mn -WA cblockbasics8_pp.cpp kernel32.lib
//
// See also cblockbasics8_pp_dmc.cmd batch command file for easy DMC compile
// from File Explorer.
//
#include "cblockbasics8.h"
#include <stdio.h>

// file handles
FILE *ppLog = stdout;   // post-processing messages output (default stdout)
FILE *inFile;           // binary input file
FILE *outFile;          // CSV output file

// some messages
const char *pgmName = "CBlockBasics8_pp.exe";
const char *verStr  = "\n%s v1.0\n";
const char *syntaxStr =
    "\nSyntax: %s InFile OutFile [LogFile]\n"
    "Where:\n"
    "  InFile:  File name for binary data.\n"
    "  OutFile: File name for output CSV data.\n"
    "  LogFile: Optional file name to log messages.  If not specified, "
    "messages are sent to stdout.\n"
    "           (You can pass \"nul\" to suppress output.)\n\n"
    "File names may contain absolute or relative paths.\n"
    "Program returns 0 on success or non-zero on error.\n";
const char *parmStr = "Input:  \"%s\"\nOutput: \"%s\"\n";

// doProcessing() -- generate CVS from binary file.  returns number of records
// processed.
int generateCSV(void) {
  BinData inBuf;
  int     cnt = 0;

  // write a header to CSV
  const char *hdr =
      "\"Rec #\",\"t\",\"vin\",\"vout\",\"InstName\",\"InstNbr\",\"StepNbr\","
      "\"TotSteps\",\"StepVal\",\"SimPtCnt\"\n";
  fprintf(outFile, hdr);

  // read/convert/write until EOF
  fread(&inBuf, sizeof(inBuf), 1, inFile);
  while (!feof(inFile)) {
    cnt++;
    fprintf(outFile, "%i,%e,%e,%e,\"%s\",%i,%i,%i,%i,%i\n", cnt,
        inBuf.instData.lastT, inBuf.instData.lastVin, inBuf.instData.lastVout,
        inBuf.instName, inBuf.instData.instNbr, inBuf.instData.stepNbr,
        inBuf.instData.totSteps, inBuf.instData.stepVal,
        inBuf.instData.simPtCnt);
    fread(&inBuf, sizeof(inBuf), 1, inFile);
  }

  return cnt;
}

// show syntax
void showSyntax(const char *pgmArg) {
  printf(verStr, pgmArg);
  printf(syntaxStr, pgmArg);
}

//*------------------------------------------------------------------------------
// main() - returns zero on success or non-zero code on failure
//*------------------------------------------------------------------------------
int main(int argc, char **argv) {
  // for convenience
  const char *pgmArg = argv[0];
  const char *inArg  = argv[1];
  const char *outArg = argv[2];

  // if argument count invalid, show syntax on stdout and exit
  if (argc < 3 || argc > 4) {
    showSyntax(pgmName);
    return -1;
  }

  // if log parameter is supplied, open the file for create/replace
  if (argc == 4) {
    ppLog = fopen(argv[3], "w+");
    if (!ppLog) {
      return -2;   // can't do much except return error
    }
  }

  // log info so far
  fprintf(ppLog, verStr, pgmArg);
  fprintf(ppLog, parmStr, inArg, outArg);

  // open input file for read/binary
  inFile = fopen(inArg, "rb");
  if (!inFile) {
    fprintf(ppLog, "Unable to open input file.\n");
    fclose(ppLog);
    return -3;
  }

  // open output file for create/replace
  outFile = fopen(outArg, "w+");
  if (!inFile) {
    fprintf(ppLog, "Unable to open output file.\n");
    fclose(inFile);
    fclose(ppLog);
    return -4;
  }

  // do actual processing to convert file
  int cnt = generateCSV();

  // log record count
  fprintf(ppLog, "%i records processed.\n", cnt);

  // close files
  fclose(outFile);
  fclose(inFile);
  fclose(ppLog);

  // return success
  return 0;
}

/*******************************************************************************
 * EOF CBlockBasics8_pp.cpp
 ******************************************************************************/
