/*************************************************************
 * 
 * SimpleOpFlashAna class
 * 
 * This is a simple class that we can use for writing out
 * a TTree with interesting information from OpFlash obejcts.
 *
 * Wesley Ketchum (wketchum@fnal.gov), Aug29, 2016
 * 
 *************************************************************/

//some standard C++ includes
#include <vector>

//some ROOT includes
#include "TTree.h"
#include "TH1F.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/OpFlash.h"
#include "lardataobj/RecoBase/OpHit.h"

namespace opdet { class SimpleOpFlashAna; }

const int MAXOPHIT = 50;

class opdet::SimpleOpFlashAna {

public:
    
  SimpleOpFlashAna(){}
  
  void InitROOTObjects(TTree *tree,TH1F* hist);
  void ProcessFlashes(std::vector<recob::OpFlash> const&,
		      std::vector< std::vector<recob::OpHit const*> > const& );
  
private:
  
  //let's make a useful struct for our output tree!
  typedef struct OpFlashTreeObj{
    double time;
    double pe;
    double y;
    double z;
    int    n_hits;
    int    n_hits_2pe;
    
    double ophit_time[MAXOPHIT];
    double ophit_pe[MAXOPHIT];
    int    ophit_chan[MAXOPHIT];
    
    void Clear() {
      time=-99999999; pe=-9999; y=-9999; z=-9999; n_hits=-1; n_hits_2pe=-1;
      for(int i=0; i<MAXOPHIT; ++i)
	{ ophit_time[i]=-99999999; ophit_pe[i] = -9999; ophit_chan[i] = -1; }
    }
    OpFlashTreeObj() { Clear(); }
  } OpFlashTreeObj_t;
  
  OpFlashTreeObj_t fFlashVals;
  TTree*           fFlashAnaTree;
  TH1F*            fHistFlashPerEv;
};
