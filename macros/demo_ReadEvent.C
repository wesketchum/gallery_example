/*************************************************************
 * 
 * demo_ReadEvent() macro
 * 
 * This is a simple demonstration of reading a LArSoft file 
 * and printing out the run and event numbers. You can also
 * put the event numbers into a histogram!
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

//convenient for us! let's not bother with art and std namespaces!
using namespace art;
using namespace std;

void demo_ReadEvent() {

  //By default, Wes hates the stats box! But by default, Wes forgets to disable it in his ROOT profile stuff...
  gStyle->SetOptStat(0);

  //Let's make a histogram to store event numbers.
  //I ran this before, so I know my event range. You can adjust this for your file!
  TH1F* h_events = new TH1F("h_events","Event Numbers;event;N_{events} / bin",100,4500,5000); 
  
  //We specify our files in a list of file names!
  //Note: multiple files allowed. Just separate by comma.
  vector<string> filenames { "MyInputFile_1.root" };

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

    //ok, then we can fill our histogram!
    h_events->Fill(ev.eventAuxiliary().event());

  } //end loop over events!


  //now, we're in a macro: we can just draw the histogram!
  h_events->Draw();

  //and ... done!
}
