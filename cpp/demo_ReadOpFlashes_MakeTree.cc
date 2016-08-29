/*************************************************************
 * 
 * demo_ReadOpFlashes_MakeTree program
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::OpFlash information, and accessing
 * associated recob::OpHit information. This one makes a TTree
 * to store output results!
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
#include "TClonesArray.h"

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

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

using namespace std::chrono;

//let's make a useful struct for our output tree!
const int MAXOPHIT = 50;
struct OpFlashTreeObj{
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
};

int main() {

  TFile f_output("demo_ReadOpFlashes_output.root","RECREATE");

  //OK, setup our tree info now
  OpFlashTreeObj flash_vals;

  TTree* flashanatree = new TTree("flashanatree","MyFlashAnaTree");
  flashanatree->Branch("flash",&flash_vals,"time/D:pe/D:y/D:z/D:n_hits/I:n_hits_2pe/I");
  flashanatree->Branch("ophit_time",&flash_vals.ophit_time,"ophit_time[n_hits]/D");
  flashanatree->Branch("ophit_pe",&flash_vals.ophit_pe,"ophit_pe[n_hits]/D");
  flashanatree->Branch("ophit_chan",&flash_vals.ophit_chan,"ophit_chan[n_hits]/I");
  

  //still gonna make this historgram
  TH1F* h_flash_per_ev = new TH1F("h_flash_per_ev","OpFlashes per event;N_{flashes};Events / bin",20,-0.5,19.5); 
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { "MyInputFile_1.root" };

  //We need to specify the "input tag" for our collection of optical flashes.
  //This is like the module label, except it can also include process name
  //and an instance label. Format is like this:
  //InputTag mytag{ "module_label","instance_label","process_name"};
  //You can ignore instance label if there isn't one. If multiple processes
  //used the same module label, the most recent one should be used by default.
  //
  //Check the contents of your file by setting up a version of uboonecode, and
  //running an event dump:
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1 | grep opflash '
  InputTag opflash_tag { "opflashSat" };


  //ok, now for the event loop! Here's how it works.
  //
  //gallery has these built-in iterator things.
  //
  //You declare an event with a list of file names. Then, you
  //move to the next event by using the "next()" function.
  //Do that until you are "atEnd()".
  //
  //In a for loop, that looks like this:

  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    auto t_begin = high_resolution_clock::now();
    
    //to get run and event info, you use this "eventAuxillary()" object.
    cout << "Processing "
	 << "Run " << ev.eventAuxiliary().run() << ", "
	 << "Event " << ev.eventAuxiliary().event() << endl;

    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::OpFlash>* object.
    auto const& opflash_handle = ev.getValidHandle<vector<recob::OpFlash>>(opflash_tag);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    auto const& opflash_vec(*opflash_handle);

    //For good measure, print out the number of optical hits
    cout << "\tThere are " << opflash_vec.size() << " OpFlashes in this event." << endl;
    
    //We can fill our histogram for number of op hits now!!!
    h_flash_per_ev->Fill(opflash_vec.size());

    //We're gonna do this a tad differently now. Let's setup the FindMany, and run our loop
    //over the handle, so we only do one loop;
    FindMany<recob::OpHit> ophits_per_flash(opflash_handle,ev,opflash_tag);
    for (size_t i_f = 0, size_flash = opflash_vec.size(); i_f != size_flash; ++i_f) {

      std::vector<recob::OpHit const*> ophits_vec; //this will hold the output. Note it's a vec of ptrs.
      ophits_per_flash.get(i_f,ophits_vec); //This fills the output usting the findmany object.

      //initialize/clear out our tree objects
      flash_vals.Clear();
      //ophit_ar.Clear();
      //ophit_vals.resize(ophits_vec.size(),new OpHitTreeObj());

      //fill some flash info
      auto const& myflash = opflash_vec[i_f];
      flash_vals.time = myflash.Time();
      flash_vals.pe = myflash.TotalPE();
      flash_vals.y = myflash.YCenter();
      flash_vals.z = myflash.ZCenter();

      flash_vals.n_hits = ophits_vec.size();

      //loop over the optical hits, and fill that info too
      flash_vals.n_hits_2pe=0;
      for(size_t i_oph=0, size_hits = ophits_vec.size(); i_oph!=size_hits; ++i_oph){
	if(ophits_vec[i_oph]->PE()>2) ++flash_vals.n_hits_2pe;
	flash_vals.ophit_time[i_oph] = ophits_vec[i_oph]->PeakTime();
	flash_vals.ophit_pe[i_oph]   = ophits_vec[i_oph]->PE();
	flash_vals.ophit_chan[i_oph] = ophits_vec[i_oph]->OpChannel();
	cout << "\t\tOpChannel is " << ophits_vec[i_oph]->OpChannel() << " " << flash_vals.ophit_chan[i_oph] << endl;
      }

      //fill the tree. set branch address on ophits to be safe.
      //ophitbranch->SetBranchAddress(&ophit_vals);
      flashanatree->Fill();
      /*
      //we should delete our pointers, since we new'ed them.
      for(auto & ptr : ophit_vals)
	delete ptr;
      ophit_vals.clear();
      */
    } //end loop over flashes
    
    
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
  } //end loop over events!


  //and ... write to file!
  f_output.Write();
  f_output.Close();

}
