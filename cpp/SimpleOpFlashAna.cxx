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


#include "SimpleOpFlashAna.hh"

void opdet::SimpleOpFlashAna::InitROOTObjects(TTree *tree, TH1F* hist)
{
  fFlashAnaTree = tree;
  fFlashAnaTree->SetName("flashanatree");
  fFlashAnaTree->SetTitle("MyFlashAnaTree");
  fFlashAnaTree->Branch("flash",&fFlashVals,"time/D:pe/D:y/D:z/D:n_hits/I:n_hits_2pe/I");
  fFlashAnaTree->Branch("ophit_time",&fFlashVals.ophit_time,"ophit_time[n_hits]/D");
  fFlashAnaTree->Branch("ophit_pe",&fFlashVals.ophit_pe,"ophit_pe[n_hits]/D");
  fFlashAnaTree->Branch("ophit_chan",&fFlashVals.ophit_chan,"ophit_chan[n_hits]/I");

  fHistFlashPerEv = hist;
  fHistFlashPerEv->SetName("h_flash_per_ev");
  fHistFlashPerEv->SetTitle("OpFlashes per event;N_{flashes};Events / bin");
  fHistFlashPerEv->SetBins(20,-0.5,19.5);
}

void opdet::SimpleOpFlashAna::ProcessFlashes(std::vector<recob::OpFlash> const& opflash_vec,
					     std::vector< std::vector<recob::OpHit const*> > const& ophits_vecs){

  fHistFlashPerEv->Fill(opflash_vec.size());

  for (size_t i_f = 0, size_flash = opflash_vec.size(); i_f != size_flash; ++i_f) {
    
    //initialize/clear out our tree objects
    fFlashVals.Clear();

    //fill some flash info
    auto const& myflash = opflash_vec[i_f];
    fFlashVals.time = myflash.Time();
    fFlashVals.pe = myflash.TotalPE();
    fFlashVals.y = myflash.YCenter();
    fFlashVals.z = myflash.ZCenter();

    auto const& ophits_vec = ophits_vecs[i_f];    
    fFlashVals.n_hits = ophits_vec.size();
    
    //loop over the optical hits, and fill that info too
    fFlashVals.n_hits_2pe=0;
    for(size_t i_oph=0, size_hits = ophits_vec.size(); i_oph!=size_hits; ++i_oph){
      if(ophits_vec[i_oph]->PE()>2) ++fFlashVals.n_hits_2pe;
      fFlashVals.ophit_time[i_oph] = ophits_vec[i_oph]->PeakTime();
      fFlashVals.ophit_pe[i_oph]   = ophits_vec[i_oph]->PE();
      fFlashVals.ophit_chan[i_oph] = ophits_vec[i_oph]->OpChannel();
    }
    
    //fill the tree. set branch address on ophits to be safe.
    fFlashAnaTree->Fill();
    
  } //end loop over flashes
  
}

