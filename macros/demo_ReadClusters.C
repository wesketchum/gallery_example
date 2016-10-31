/*************************************************************
 * 
 * demo_ReadClusters() macro
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::Cluster information, and accessing
 * associated recob::Hit information.
 *
 * To run this, open root, and do:
 * root [0] .L demo_ReadClusters.C++
 * root [1] demo_ReadClusters()
 *
 * Wesley Ketchum (wketchum@fnal.gov), Oct31, 2016
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
#include "TCanvas.h"
#include "TStyle.h"
#include "TStopwatch.h"

//"art" includes (canvas, and gallery)
#include "canvas/Utilities/InputTag.h"
#include "gallery/Event.h"
#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

//"larsoft" object includes
#include "lardataobj/RecoBase/Cluster.h"
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

void demo_ReadClusters() {

  //By default, Wes hates the stats box! But by default, Wes forgets to disable it in his ROOT profile stuff...
  gStyle->SetOptStat(0);

  //Let's make a histograms to store information!
  TH1F* h_cluster_per_ev = new TH1F("h_cluster_per_ev","Clusters per event;N_{clusters};Events / bin",100,-0.5,99.5); 
  TH1F* h_cluster_integral_sum = new TH1F("h_integral_sum","Clusters; Integral Sum; Events / 100 ADC counts",200,0,20000);
  TH1F* h_cluster_integral_ave = new TH1F("h_integral_ave","Clusters; Integral Average; Events / 100 ADC counts",200,0,20000);
  TH1F* h_hits_per_cluster = new TH1F("h_hits_per_cluster","Hits per Cluster;N_{hits};Events / bin",100,0,500);
  TH1F* h_hits_per_cluster_75 = new TH1F("h_hits_per_cluster_75","Hits (Integral > 75 ADC) per Cluster;N_{hits};Events / bin",100,0,500);
  
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
  //  'lar -c eventdump.fcl -s MyInputFile_1.root -n 1 | grep "std::vector<recob::Cluster>" '
  InputTag cluster_tag { "pandora" };


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
    //vector<recob::Cluster>* object.
    auto const& cluster_handle = ev.getValidHandle<vector<recob::Cluster>>(cluster_tag);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    
    auto const& cluster_vec(*cluster_handle);

    //For good measure, print out the number of optical hits
    cout << "\tThere are " << cluster_vec.size() << " Clusters in this event." << endl;
    
    //We can fill our histogram for number of op hits now!!!
    h_cluster_per_ev->Fill(cluster_vec.size());

    //We can loop over the vector to get optical hit info too!
    //
    //Don't remember what's in a recob::OpHit? Me neither. So, we can look it up.
    //The environment variable $LARDATAOBJ_DIR contains the directory of the
    //lardataobj product we set up. Inside that directory we can find what we need:
    // ' ls $LARDATAOBJ_DIR/source/lardataobj/RecoBase/Cluster.h '
    //Note: it's the same directory structure as the include, after you go into
    //the 'source' directory. Look at that file and see what you can access.
    //
    //So, let's fill our histograms for the opflash info
    //We can use a range-based for loop for ease.
    for( auto const& cluster : cluster_vec){
      h_cluster_integral_sum->Fill(cluster.Integral());
      h_cluster_integral_ave->Fill(cluster.IntegralAverage());
    }

    //We can also grab associated Hits per Cluster!
    //This is done via the "FindMany" object. It looks something like this:
    //
    // FindMany<TObj> tobjs_per_uobjs(uobj_handle,ev,input_tag)
    //
    // TObj is the object you want to grab. UObj_handle is the handle to the
    // associated object. 'ev' is the art event and 'input_tag' is the input
    // tag for the module/instance/process that made the association.
    // So, in our case, we want recob::Hit objects per our cluster_handle. The
    // associations were made by the same modules that made the clusters. so:
    FindMany<recob::Hit> hits_per_cluster(cluster_handle,ev,cluster_tag);

    //Now, we need to loop over the flashes and get the collection (vector) of
    //associated hits per flash. That goes something like this:
    for (size_t i_c = 0, size_cluster = cluster_handle->size(); i_c != size_cluster; ++i_c) {

      std::vector<recob::Hit const*> hits_vec; //this will hold the output. Note it's a vec of ptrs.
      hits_per_cluster.get(i_c,hits_vec); //This fills the output using the findmany object.

      //now we can fill our n_hits per cluster!
      h_hits_per_cluster->Fill(hits_vec.size());

      //we can loop over this hit collection too!
      int nhits=0;
      for(auto const& hitptr : hits_vec)
	if(hitptr->Integral()>75) ++nhits;
      
      h_hits_per_cluster_75->Fill(nhits);
    }
    
    timer.Stop();
    cout << "\tEvent took " << timer.RealTime()*1000. << " ms to process." << endl;
  } //end loop over events!


  //now, we're in a macro: we can just draw the histogram!
  //Let's make a TCanvas to draw our two histograms side-by-side
  TCanvas* canvas = new TCanvas("canvas","Cluster Info!",1500,500);
  canvas->Divide(3); //divides the canvas in three!

  //use this function to move under/overflow into visible bins.
  ShowUnderOverFlow(h_cluster_per_ev);
  ShowUnderOverFlow(h_hits_per_cluster);
  ShowUnderOverFlow(h_hits_per_cluster_75);
  ShowUnderOverFlow(h_cluster_integral_sum);
  ShowUnderOverFlow(h_cluster_integral_ave);

  canvas->cd(1);     //moves us to the first canvas
  h_cluster_per_ev->Draw();
  canvas->cd(2);     //moves us to the second
  h_hits_per_cluster->SetLineColor(kRed);
  h_hits_per_cluster->Draw();
  h_hits_per_cluster_75->SetLineColor(kBlue);
  h_hits_per_cluster_75->Draw("same");
  canvas->cd(3);     //moves us to the third
  h_cluster_integral_sum->SetLineColor(kRed);
  h_cluster_integral_ave->SetLineColor(kBlue);
  h_cluster_integral_ave->Draw();
  h_cluster_integral_sum->Draw("same");
    
  //and ... done!
}
