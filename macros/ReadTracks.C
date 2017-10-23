/*************************************************************
 * 
 * ReadTracks() macro
 * 
 * To run this, open root, and do:
 * root [0] .L ReadTracks.C++
 * root [1] demo_ReadClusters()
 *
 * Wesley Ketchum (wketchum@fnal.gov), Oct31, 2016
 * 
 *************************************************************/


//some standard C++ includes
#include <iostream>
#include <fstream>
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
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"

//See this at $NUSIMDATA_DIR/source/nusimdata/SimulationBase/MCTruth.h
#include "nusimdata/SimulationBase/MCTruth.h"

//I like doing this to not get fooled by underflow/overflow
void ShowUnderOverFlow(TH1* h1){
  h1->SetBinContent(1, h1->GetBinContent(0)+h1->GetBinContent(1));
  h1->SetBinContent(0,0);

  int nbins = h1->GetNbinsX();
  h1->SetBinContent(nbins, h1->GetBinContent(nbins)+h1->GetBinContent(nbins+1));
  h1->SetBinContent(nbins+1,0);
}

#include <boost/algorithm/string/predicate.hpp>
std::vector<std::string> GetFileList(std::string input_file)
{
  std::vector<std::string> filenames;
  if(boost::algorithm::ends_with(input_file,".root")){
    filenames.emplace_back(input_file);
    std::cout << "Added file " << filenames.back() << " to be processed." << std::endl;
  }
  else{
    std::ifstream input(input_file);
    for(std::string line; std::getline(input,line);){
      filenames.emplace_back(line);
      std::cout << "Added file " << filenames.back() << " to be processed." << std::endl;
    }
  }
  return filenames;
}

//let's make a useful struct for our output tree!
const int MAXHITS = 10000;
struct TrackTreeObj{
  unsigned int idx;

  float startx;
  float starty;
  float startz;
  float endx;
  float endy;
  float endz;
  float length;

  float truth_startx;
  float truth_starty;
  float truth_startz;
  float truth_endx;
  float truth_endy;
  float truth_endz;

  int n_hits;  

  float hit_time[MAXHITS];
  float hit_amp[MAXHITS];
  float hit_integral[MAXHITS];
  unsigned int hit_wire[MAXHITS];
  unsigned int hit_plane[MAXHITS];
  unsigned int hit_tpc[MAXHITS];
  
  void Clear() {
    idx=999999;
    startx=-999999; starty=-999999; startz=-999999;
    endx=-999999; endy=-999999; endz=-999999;
    length = -99999;

    truth_startx=-999999; truth_starty=-999999; truth_startz=-999999;
    truth_endx=-999999; truth_endy=-999999; truth_endz=-999999;

    n_hits = -1;
    
    for(int i=0; i<MAXHITS; ++i)
      {
	hit_time[i]=-99999999; hit_amp[i] = -9999; hit_integral[i] = -9999;
	hit_wire[i]=999999; hit_plane[i]=999999; hit_tpc[i]=999999;
      }
  }
  TrackTreeObj() { Clear(); }
};

void ReadTracks(std::string input_file, std::string output_file="readtracks_output.root") {

  //By default, Wes hates the stats box! But by default, Wes forgets to disable it in his ROOT profile stuff...
  gStyle->SetOptStat(0);

  //setup output file and tree
  TFile *f_output = new TFile(output_file.c_str(),"RECREATE");
  TrackTreeObj track_vals;
  TTree* trkanatree = new TTree("trkanatree","MyTrackAnaTree");
  trkanatree->Branch("trk",&track_vals,"idx/i:startx/F:starty/F:startz/F:endx/F:endy/F:endz/F:length/F:truth_startx/F:truth_starty/F:truth_startz/F:truth_endx/F:truth_endy/F:truth_endz:n_hits/I");
  trkanatree->Branch("hit_time",&track_vals.hit_time,"hit_time[n_hits]/F");
  trkanatree->Branch("hit_amp",&track_vals.hit_amp,"hit_amp[n_hits]/F");
  trkanatree->Branch("hit_integral",&track_vals.hit_integral,"hit_integral[n_hits]/F");
  trkanatree->Branch("hit_wire",&track_vals.hit_wire,"hit_wire[n_hits]/i");
  trkanatree->Branch("hit_plane",&track_vals.hit_plane,"hit_plane[n_hits]/i");
  trkanatree->Branch("hit_tpc",&track_vals.hit_tpc,"hit_tpc[n_hits]/i");

  
  //format our file list
  std::vector<std::string> filenames = GetFileList(input_file);

  //specify the input tag
  art::InputTag track_tag { "pmAlgKalmanTrack" } ;
  art::InputTag gen_tag   { "generator" };
  
  for (gallery::Event ev(filenames) ; !ev.atEnd(); ev.next()) {

    //to get run and event info, you use this "eventAuxillary()" object.
    cout << "Processing "
	 << "Run " << ev.eventAuxiliary().run() << ", "
	 << "Event " << ev.eventAuxiliary().event() << endl;

    //Now, we want to get a "valid handle" (which is like a pointer to our collection")
    //We use auto, cause it's annoying to write out the fill type. But it's like
    //vector<recob::Track>* object.
    auto const& track_handle = ev.getValidHandle<std::vector<recob::Track>>(track_tag);

    //similarly for the MCTruth
    auto const& mctruth_handle = ev.getValidHandle<std::vector<simb::MCTruth>>(gen_tag);
    simb::MCParticle const& truth_particle = mctruth_handle->at(0).GetParticle(0);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    auto const& track_vec(*track_handle);

    //For good measure, print out the number of optical hits
    std::cout << "\tThere are " << track_vec.size() << " tracks in this event." << std::endl;
    
    //Let's setup the FindMany for associations of hits, and run our loop
    //over the handle, so we only do one loop;
    art::FindMany<recob::Hit> hits_per_track(track_handle,ev,track_tag);
    
    for( unsigned int i_trk=0; i_trk<track_vec.size(); ++i_trk){

      track_vals.Clear();
      
      std::vector<recob::Hit const*> hits_vec; //this will hold the output. Note it's a vec of ptrs.
      hits_per_track.get(i_trk,hits_vec); //This fills the output usting the findmany object.

      std::cout << "\tFound " << hits_vec.size() << " hits in track " << i_trk << std::endl;
      
      track_vals.idx = i_trk;

      recob::Track const& trk = track_vec[i_trk]; 

      track_vals.startx = trk.Start().X();
      track_vals.startx = trk.Start().Y();
      track_vals.startx = trk.Start().Z();
      track_vals.endx = trk.End().X();
      track_vals.endx = trk.End().Y();
      track_vals.endx = trk.End().Z();
      track_vals.length = trk.Length();

      track_vals.truth_startx = truth_particle.Vx();
      track_vals.truth_starty = truth_particle.Vy();
      track_vals.truth_startz = truth_particle.Vz();
      track_vals.truth_endx = truth_particle.EndX();
      track_vals.truth_endy = truth_particle.EndY();
      track_vals.truth_endz = truth_particle.EndZ();

      track_vals.n_hits = hits_vec.size();

      for(size_t i_h=0, size_hits = hits_vec.size(); i_h!=size_hits; ++i_h){
	track_vals.hit_time[i_h] = hits_vec[i_h]->PeakTime();
	track_vals.hit_amp[i_h]   = hits_vec[i_h]->PeakAmplitude();
	track_vals.hit_integral[i_h] = hits_vec[i_h]->Integral();
	track_vals.hit_wire[i_h] = hits_vec[i_h]->WireID().Wire;
	track_vals.hit_plane[i_h] = hits_vec[i_h]->WireID().Plane;
	track_vals.hit_tpc[i_h] = hits_vec[i_h]->WireID().TPC;
      }

      //fill the tree.
      trkanatree->Fill();

    }
    
  }
    
  //and ... write to file!
  f_output->Write();
  f_output->Close();

}
