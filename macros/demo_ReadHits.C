/*************************************************************
 * 
 * demo_ReadHits() macro
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::Hit information.
 *
 * To run this, open root, and do:
 * root [0] .L demo_ReadHits.C++
 * root [1] demo_ReadHits()
 *
 * Wesley Ketchum (wketchum@fnal.gov), Oct31, 2016
 * 
 *************************************************************/


//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TStyle.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/Hit.h"

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

void demo_ReadHits() {

  //By default, Wes hates the stats box! But by default, Wes forgets to disable it in his ROOT profile stuff...
  gStyle->SetOptStat(0);

  //Let's make a histograms to store hit information!
  TH1F* h_hits_per_ev = new TH1F("h_hits_per_ev","Hits per event;N_{hits};Events / bin",100,0,100000); 
  TH1F* h_hit_integral = new TH1F("h_hit_integral","Hit Integral Charge; ADC counts; Events / 2 ADC",200,0,400);
  TH1F* h_hit_peaktime = new TH1F("h_hit_peaktime","Hit Peak Time; t (TDC counts); Events / 10 TDC counts",1000,-1000,9000);
  TH1F* h_hit_peakamp = new TH1F("h_hit_peakamp","Hit Peak Amplitude Charge; ADC counts; Events / 2 ADC",200,0,400);
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { "MyInputFile_1.root" };

  //We need to specify the "input tag" for our collection of optical hits.
  //This is like the module label, except it can also include process name
  //and an instance label. Format is like this:
  //InputTag mytag{ "module_label","instance_label","process_name"};
  //You can ignore instance label if there isn't one. If multiple processes
  //used the same module label, the most recent one should be used by default.
  //
  //Check the contents of your file by setting up a version of larsoft, and
  //running an event dump:
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1 | grep "std::vector<recob::Hit>" '
  InputTag hit_tag { "gaushit" };


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

    //to get run and event info, you use this "eventAuxillary()" object.
    cout << "Processing "
	 << "Run " << ev.eventAuxiliary().run() << ", "
	 << "Event " << ev.eventAuxiliary().event() << endl;

    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::Hit>* object.
    auto const& hit_handle = ev.getValidHandle<vector<recob::Hit>>(hit_tag);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    auto const& hit_vec(*hit_handle);

    //For good measure, print out the number of hits
    cout << "\tThere are " << hit_vec.size() << " Hits in this event." << endl;
    
    //We can fill our histogram for number of hits now!!!
    h_hits_per_ev->Fill(hit_vec.size());

    //We can loop over the vector to get hit info too!
    //
    //Don't remember what's in a recob::Hit? Me neither. So, we can look it up.
    //The environment variable $LARDATAOBJ_DIR contains the directory of the
    //lardataobj product we set up. Inside that directory we can find what we need:
    // ' ls $LARDATAOBJ_DIR/source/lardataobj/RecoBase/Hit.h '
    //Note: it's the same directory structure as the include, after you go into
    //the 'source' directory. Look at that file and see what you can access.
    //
    //So, let's fill our histogram for the hit integral/peak time/peak amplitude.
    //We can use a range-based for loop for ease.
    for( auto const& hit : hit_vec){
      h_hit_integral->Fill(hit.Integral());
      h_hit_peaktime->Fill(hit.PeakTime());
      h_hit_peakamp->Fill(hit.PeakAmplitude());
    }


    
  } //end loop over events!


  //now, we're in a macro: we can just draw the histogram!
  //Let's make a TCanvas to draw our two histograms side-by-side
  TCanvas* canvas = new TCanvas("canvas","Hit Info!",1500,500);
  canvas->Divide(3); //divides the canvas in three!
  canvas->cd(1);     //moves us to the first canvas
  h_hits_per_ev->Draw();
  canvas->cd(2);     //moves us to the second canvas
  ShowUnderOverFlow(h_hit_peaktime); //use this function to move under/overflow into visible bins. 
  h_hit_peaktime->Draw();
  canvas->cd(3);     //moves us to the third canvas
  ShowUnderOverFlow(h_hit_peakamp); //use this function to move under/overflow into visible bins. 
  ShowUnderOverFlow(h_hit_integral); //use this function to move under/overflow into visible bins. 
  h_hit_integral->SetLineColor(kRed);
  h_hit_peakamp->SetLineColor(kBlue);
  h_hit_peakamp->Draw();
  h_hit_integral->Draw("same");
  

  //and ... done!
}
