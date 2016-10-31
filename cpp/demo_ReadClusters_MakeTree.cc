/*************************************************************
 * 
 * demo_ReadClusters_MakeTree program
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and accessing recob::Cluster information, and accessing
 * associated recob::Hit information. This one makes a TTree
 * to store output results!
 *
 * Wesley Ketchum (wketchum@fnal.gov), Oct31, 2016
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
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"

//our own includes!
#include "hist_utilities.h"

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

using namespace std::chrono;

//let's make a useful struct for our output tree!
const int MAXHIT = 10000;
struct ClusterTreeObj{
  float integral_sum;
  float integral_ave;
  float integral_std;
  int    n_hits;
  int    n_hits_75;
  unsigned int index;

  float hit_time[MAXHIT];
  float hit_amp[MAXHIT];
  float hit_integral[MAXHIT];
  
  void Clear() {
    integral_sum=-9999; integral_ave=-9999; integral_std=-9999; n_hits=-1; n_hits_75=-1; index=999999;
    for(int i=0; i<MAXHIT; ++i)
      { hit_time[i]=-99999999; hit_amp[i] = -9999; hit_integral[i] = -9999; }
  }
  ClusterTreeObj() { Clear(); }
};

int main() {

  TFile f_output("demo_ReadClusters_output.root","RECREATE");

  //OK, setup our tree info now
  ClusterTreeObj cluster_vals;

  TTree* clusteranatree = new TTree("clusteranatree","MyClusterAnaTree");
  clusteranatree->Branch("cluster",&cluster_vals,"integral_sum/F:integral_ave/F:integral_std/F:n_hits/I:n_hits_75/I:index/i");
  clusteranatree->Branch("hit_time",&cluster_vals.hit_time,"hit_time[n_hits]/F");
  clusteranatree->Branch("hit_amp",&cluster_vals.hit_amp,"hit_amp[n_hits]/F");
  clusteranatree->Branch("hit_integral",&cluster_vals.hit_integral,"hit_integral[n_hits]/F");
  

  //still gonna make this historgram
  TH1F* h_cluster_per_ev = new TH1F("h_cluster_per_ev","Clusters per event;N_{clusters};Events / bin",100,-0.5,99.5); 
  
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
  InputTag cluster_tag { "pandoraNu" };


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
    //vector<recob::Cluster>* object.
    auto const& cluster_handle = ev.getValidHandle<vector<recob::Cluster>>(cluster_tag);

    //We can now treat this like a pointer, or dereference it to have it be like a vector.
    //I (Wes) for some reason prefer the latter, so I always like to do ...
    
    auto const& cluster_vec(*cluster_handle);

    //For good measure, print out the number of optical hits
    cout << "\tThere are " << cluster_vec.size() << " Clusters in this event." << endl;
    
    //We can fill our histogram for number of op hits now!!!
    h_cluster_per_ev->Fill(cluster_vec.size());

    //We're gonna do this a tad differently now. Let's setup the FindMany, and run our loop
    //over the handle, so we only do one loop;
    FindMany<recob::Hit> hits_per_cluster(cluster_handle,ev,cluster_tag);
    for (size_t i_c = 0, size_cluster = cluster_vec.size(); i_c != size_cluster; ++i_c) {

      std::vector<recob::Hit const*> hits_vec; //this will hold the output. Note it's a vec of ptrs.
      hits_per_cluster.get(i_c,hits_vec); //This fills the output usting the findmany object.

      //initialize/clear out our tree objects
      cluster_vals.Clear();

      //fill some flash info
      auto const& mycluster = cluster_vec[i_c];
      cluster_vals.integral_sum = mycluster.Integral();
      cluster_vals.integral_ave = mycluster.IntegralAverage();
      cluster_vals.integral_std = mycluster.IntegralStdDev();

      cluster_vals.n_hits = hits_vec.size();
      cluster_vals.index = i_c;
      
      //loop over the optical hits, and fill that info too
      cluster_vals.n_hits_75=0;
      for(size_t i_h=0, size_hits = hits_vec.size(); i_h!=size_hits; ++i_h){
	if(hits_vec[i_h]->Integral()>75) ++cluster_vals.n_hits_75;
	cluster_vals.hit_time[i_h] = hits_vec[i_h]->PeakTime();
	cluster_vals.hit_amp[i_h]   = hits_vec[i_h]->PeakAmplitude();
	cluster_vals.hit_integral[i_h] = hits_vec[i_h]->Integral();
      }

      //fill the tree. set branch address on ophits to be safe.
      clusteranatree->Fill();

    } //end loop over flashes
    
    
    auto t_end = high_resolution_clock::now();
    duration<double,std::milli> time_total_ms(t_end-t_begin);
    cout << "\tEvent took " << time_total_ms.count() << " ms to process." << endl;
  } //end loop over events!


  //and ... write to file!
  f_output.Write();
  f_output.Close();

}
