/*************************************************************
 * 
 * demo_ReadOpFlashes() macro
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::OpFlash information, and accessing
 * associated recob::OpHit information.
 *
 * Wesley Ketchum (wketchum@fnal.gov), Aug28, 2016
 * 
 *************************************************************/


//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TFile.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/OpFlash.h"
#include "lardataobj/RecoBase/OpHit.h"

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

//I like doing this to not get fooled by underflow/overflow
void ShowUnderOverFlow( TH1* h1){
  h1->SetBinContent(1, h1->GetBinContent(0)+h1->GetBinContent(1));
  h1->SetBinContent(0,0);

  int nbins = h1->GetNbinsX();
  h1->SetBinContent(nbins, h1->GetBinContent(nbins)+h1->GetBinContent(nbins+1));
  h1->SetBinContent(nbins+1,0);
}

void demo_ReadOpFlashes() {

  //By default, Wes hates the stats box! But by default, Wes forgets to disable it in his ROOT profile stuff...
  gStyle->SetOptStat(0);

  //Let's make a histograms to store optical information!
  TH1F* h_flash_per_ev = new TH1F("h_flash_per_ev","OpFlashes per event;N_{flashes};Events / bin",20,-0.5,19.5); 
  TH1F* h_flash_pe = new TH1F("h_flash_pe","Flash PEs; PE; Events / 0.1 PE",100,0,50);
  TH1F* h_flash_y = new TH1F("h_flash_y","Flash y position; y (cm); Events / 0.1 cm",100,-200,200);
  TH1F* h_flash_z = new TH1F("h_flash_z","Flash z position; z (cm); Events / 0.1 cm",100,-100,1100);
  TH1F* h_flash_time = new TH1F("h_flash_time","Flash Time; time (#mus); Events / 0.5 #mus",60,-5,25);
  TH1F* h_ophits_per_flash = new TH1F("h_ophits_per_flash","OpHits per Flash;N_{optical hits};Events / bin",20,-0.5,19.5);
  TH1F* h_ophits_per_flash_2pe = new TH1F("h_ophits_per_flash_2pe","OpHits (> 2 PE) per Flash;N_{optical hits};Events / bin",20,-0.5,19.5);
  
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

  TStopwatch timer;
  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {
    timer.Start();
    
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

    //We can loop over the vector to get optical hit info too!
    //
    //Don't remember what's in a recob::OpHit? Me neither. So, we can look it up.
    //The environment variable $LARDATAOBJ_DIR contains the directory of the
    //lardataobj product we set up. Inside that directory we can find what we need:
    // ' ls $LARDATAOBJ_DIR/source/lardataobj/RecoBase/OpFlash.h '
    //Note: it's the same directory structure as the include, after you go into
    //the 'source' directory. Look at that file and see what you can access.
    //
    //So, let's fill our histograms for the opflash info
    //We can use a range-based for loop for ease.
    for( auto const& flash : opflash_vec){
      h_flash_pe->Fill(flash.TotalPE());
      h_flash_y->Fill(flash.YCenter());
      h_flash_z->Fill(flash.ZCenter());
      h_flash_time->Fill(flash.Time());
    }

    //We can also grab associated OpHits per OpFlash!
    //This is done via the "FindMany" object. It looks something like this:
    //
    // FindMany<TObj> tobjs_per_uobjs(uobj_handle,ev,input_tag)
    //
    // TObj is the object you want to grab. UObj_handle is the handle to the
    // associated object. 'ev' is the art event and 'input_tag' is the input
    // tag for the module/instance/process that made the association.
    // So, in our case, we want recob::OpHit objects per our flash_handle. The
    // associations were made by the same modules that made the flashes. so:
    FindMany<recob::OpHit> ophits_per_flash(opflash_handle,ev,opflash_tag);

    //Now, we need to loop over the flashes and get the collection (vector) of
    //associated hits per flash. That goes something like this:
    for (size_t i_f = 0, size_flash = opflash_handle->size(); i_f != size_flash; ++i_f) {

      std::vector<recob::OpHit const*> ophits_vec; //this will hold the output. Note it's a vec of ptrs.
      ophits_per_flash.get(i_f,ophits_vec); //This fills the output usting the findmany object.

      //now we can fill our n_ophits per flash!
      h_ophits_per_flash->Fill(ophits_vec.size());

      //we can loop over this ophit collection too!
      int nhits=0;
      for(auto const& ophitptr : ophits_vec)
	if(ophitptr->PE()>2) ++nhits;
      
      h_ophits_per_flash_2pe->Fill(nhits);
    }
    
    timer.Stop();
    cout << "\tEvent took " << timer.RealTime()*1000. << " ms to process." << endl;
  } //end loop over events!


  //now, we're in a macro: we can just draw the histogram!
  //Let's make a TCanvas to draw our two histograms side-by-side
  TCanvas* canvas = new TCanvas("canvas","OpFlash Info!",1000,500);
  canvas->Divide(2); //divides the canvas in two!

  //use this function to move under/overflow into visible bins.
  ShowUnderOverFlow(h_flash_per_ev);
  ShowUnderOverFlow(h_ophits_per_flash);

  canvas->cd(1);     //moves us to the first half of canvas
  h_flash_per_ev->Draw();
  canvas->cd(2);     //moves us to the second half
  h_ophits_per_flash->Draw();

  //now let's make a 2x2 canvas for our flash information
  TCanvas* c1 = new TCanvas("c1","MyCanvas",1000,1000);
  c1->Divide(2,2);

  ShowUnderOverFlow(h_flash_pe);
  ShowUnderOverFlow(h_flash_y);
  ShowUnderOverFlow(h_flash_z);
  ShowUnderOverFlow(h_flash_time);

  c1->cd(1); h_flash_pe->Draw();
  c1->cd(2); h_flash_time->Draw();
  c1->cd(3); h_flash_y->Draw();
  c1->cd(4); h_flash_z->Draw();
  
  
  //and ... done!
}
