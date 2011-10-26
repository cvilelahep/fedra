//-- Author :  Valeri Tioukov   11/06/2008

#include <string.h>
#include <iostream>
#include "EdbLog.h"
#include "EdbScanProc.h"

using namespace std;

void print_help_message()
{
  cout<< "\nUsage: \n\t  makescanset -set=setID  [-suff=suffix -dz=DZ -o=DATA_DIRECTORY -v=DEBUG] \n";
  cout<< "\t\t  suffix   - file suffix to be searched - (default: raw.root \n)";
  cout<< "\t\t  DZ       - plate-to-plate step (default = -1300.)\n";
  cout<< "\t\t  DEBUG    - verbosity level: 0-print nothing, 1-errors only, 2-normal, 3-print all messages\n";
  cout<< "\t\t -noaff    - do not read aff-files - the default is yes \n";
  cout<< "\t\t -resetaff - recreate aff-files using values from set.root\n";
  cout<< "\t\t -resetpar - reset pxxx/x.x.x.x.par-files (shrinkage correction, etc) using values from set.root\n";
  cout<< "\t\t -reset    - reset *.aff.par and *.par files\n";
  cout<< "\nExample: \n";
  cout<< "\t  makescanset -set=4554.0.1.1000 -o=/scratch/BRICKS \n";
  cout<< "\n If the data location directory if not explicitly define\n";
  cout<< "\t the current directory will be assumed to be the brick directory \n";
  cout<<endl;
}

int main(int argc, char* argv[])
{
  if (argc < 2)   { print_help_message();  return 0; }
  
  gEDBDEBUGLEVEL        = 2;

  const char *outdir    = "..";
  const char *suff    = "raw.root";
  bool        do_set      = false;
  bool        noaff       = false;
  bool        reset       = false;
  bool        resetaff    = false;
  bool        resetpar    = false;
  EdbID       id;
  int         from_plate  = 57;
  int         to_plate    =  1;
  float       z0          =  0;
  float       dz          = -1300;

  for(int i=1; i<argc; i++ ) {
    char *key  = argv[i];

    if(!strncmp(key,"-set=",5))
      {
	if(strlen(key)>5)	id.Set(key+5);
	do_set=true;
      }
    else if(!strncmp(key,"-o=",3)) 
      {
	if(strlen(key)>3)	outdir=key+3;
      }
    else if(!strncmp(key,"-suff=",6)) 
      {
	if(strlen(key)>6)	suff=key+6;
      }
    else if(!strncmp(key,"-noaff",6)) 
      {
	noaff=true;
      }
    else if(!strncmp(key,"-reset",6)) 
      {
	reset=true;
      }
    else if(!strncmp(key,"-resetaff",9)) 
      {
	resetaff=true;
      }
    else if(!strncmp(key,"-resetpar",9)) 
      {
	resetpar=true;
      }
    else if(!strncmp(key,"-v=",3))
      {
	if(strlen(key)>3)	gEDBDEBUGLEVEL = atoi(key+3);
      }
    else if(!strncmp(key,"-dz=",4))
      {
	if(strlen(key)>4)	dz = atof(key+4);
      }
  }

  if(!do_set) print_help_message();
  else {
    
    EdbScanProc sproc;
    sproc.eProcDirClient=outdir;
    if(id.eBrick<1) return 0;
    id.Print();
    EdbScanSet sc(id.eBrick);
    sc.MakeNominalSet(id,from_plate, to_plate, z0, dz);
    sproc.MakeScannedIDList( id, sc, 60, 0, suff);
    sproc.WriteScanSet(id,sc);
    if     (resetaff) sproc.MakeAFFSet(sc);
    else if(resetpar) sproc.MakeParSet(sc);
    else if(reset)    sproc.PrepareSetStructure(sc);
    else {if(!noaff)  {
      sproc.UpdateSetWithPlatePar(id);
      sproc.UpdateSetWithAff(id,id);
    }}
    if(gEDBDEBUGLEVEL>1) sproc.ReadScanSet(id)->Print();
  }

}


