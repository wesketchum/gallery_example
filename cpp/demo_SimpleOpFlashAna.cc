/*************************************************************
 * 
 * demo_SimpleOpFlashAna program
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::OpFlash information, and accessing
 * associated recob::OpHit information. This one makes a TTree
 * to store output results!
 *
 * This uses our new SimpleOpFlashAna class
 *
 * Wesley Ketchum (wketchum@fnal.gov), Aug28, 2016
 * 
 *************************************************************/


//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/OpFlash.h"
#include "lardataobj/RecoBase/OpHit.h"

//our own includes!
#include "hist_utilities.h"

#include "SimpleOpFlashAna.hh"

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

using namespace std::chrono;
int main() {

  TFile f_output("demo_SimpleOpFlashAna_output.root","RECREATE");

  TTree* mytree = new TTree("mytree","MyTree");  
  TH1F*  myhist = new TH1F("myhist","MyHist",10,0,1);

  opdet::SimpleOpFlashAna anaAlg;
  anaAlg.InitROOTObjects(mytree,myhist);
  
  //We specify our files in a list of file names, and our input tag
  vector<string> filenames { "MyInputFile_1.root" };
  InputTag opflash_tag { "opflashSat" };

  //ok, now for the event loop!
  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    auto t_begin = high_resolution_clock::now();
    
    //to get run and event info, you use this "eventAuxillary()" object.
    cout << "Processing "
	 << "Run " << ev.eventAuxiliary().run() << ", "
	 << "Event " << ev.eventAuxiliary().event() << endl;

    //let's get a valid handle, and a vector of objects from it
    auto const& opflash_handle = ev.getValidHandle<vector<recob::OpFlash>>(opflash_tag);
    auto const& opflash_vec(*opflash_handle);

    //note, we need to get the ophit associations before running the alg.

    //prepare the container
    std::vector< std::vector<recob::OpHit const*> > ophits_vecs(opflash_vec.size());

    //run the FindMany stuff
    FindMany<recob::OpHit> ophits_per_flash(opflash_handle,ev,opflash_tag);
    for (size_t i_f = 0, size_flash = opflash_vec.size(); i_f != size_flash; ++i_f)
      ophits_per_flash.get(i_f,ophits_vecs[i_f]);

    //fill our trees in our ana alg!
    anaAlg.ProcessFlashes(opflash_vec,ophits_vecs);
    
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
  } //end loop over events!


  //and ... write to file!
  f_output.Write();
  f_output.Close();

}
