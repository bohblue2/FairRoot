/********************************************************************************
 *    Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "FairTutPropTrackFinder.h"

#include "FairLogger.h"              // for FairLogger, MESSAGE_ORIGIN
#include "FairRootManager.h"         // for FairRootManager
#include "FairTutPropHit.h"          // for FairTutPropHit
#include "FairTutPropTrack.h"        // for FairTutPropTrack

#include <TClonesArray.h>   // for TClonesArray
#include <TRandom.h>        // for TRandom, gRandom
#include <TVector3.h>       // for TVector3

FairTutPropTrackFinder::FairTutPropTrackFinder()
    : FairTask("FairTutPropTrackFinder")
    , fHitsArray1Name("FairTutPropHits")
    , fHitsArray2Name("FairTutPropHits2")
    , fHitsArray1(nullptr)
    , fHitsArray2(nullptr)
    , fTracksArray(new TClonesArray("FairTutPropTrack", 100))
{
    LOG(debug) << "Default Constructor of FairTutPropTrackFinder";
}

FairTutPropTrackFinder::~FairTutPropTrackFinder()
{
    LOG(debug) << "Destructor of FairTutPropTrackFinder";
    fTracksArray->Delete();
    delete fTracksArray;
}

void FairTutPropTrackFinder::SetParContainers()
{
    LOG(debug) << "SetParContainers of FairTutPropTrackFinder";
    // Load all necessary parameter containers from the runtime data base
    /*
    FairRunAna* ana = FairRunAna::Instance();
    FairRuntimeDb* rtdb=ana->GetRuntimeDb();

    <FairTutPropTrackFinderDataMember> = (<ClassPointer>*)
      (rtdb->getContainer("<ContainerName>"));
    */
}

InitStatus FairTutPropTrackFinder::Init()
{
    LOG(debug) << "Initilization of FairTutPropTrackFinder";

    // Get a handle from the IO manager
    FairRootManager* ioman = FairRootManager::Instance();

    // Get a pointer to the previous already existing data level
    fHitsArray1 = static_cast<TClonesArray*>(ioman->GetObject(fHitsArray1Name.data()));
    fHitsArray2 = static_cast<TClonesArray*>(ioman->GetObject(fHitsArray2Name.data()));
    if (!fHitsArray1 || !fHitsArray2) {
        LOG(error) << "No InputData array!";
        LOG(error) << "FairTutPropTrackFinder will be inactive";
        return kERROR;
    }

    // Create the TClonesArray for the output data and register
    // it in the IO manager
    ioman->Register("FairTutPropTracks", "TutProp", fTracksArray, kTRUE);

    // Do whatever else is needed at the initilization stage
    // Create histograms to be filled
    // initialize variables

    return kSUCCESS;
}

InitStatus FairTutPropTrackFinder::ReInit()
{
    LOG(debug) << "Reinitilization of FairTutPropTrackFinder";
    return kSUCCESS;
}

void FairTutPropTrackFinder::Exec(Option_t* /*option*/)
{
    fTracksArray->Delete();

    std::map<int,int> trackNofHitsMap;

    int nofHits1 = fHitsArray1->GetEntries();
    int nofHits2 = fHitsArray2->GetEntries();

    for ( int ihit1 = 0 ; ihit1 < nofHits1 ; ihit1++ ) {
        FairTutPropHit* hit1 = static_cast<FairTutPropHit*>(fHitsArray1->At(ihit1));
        auto trackFromMap = trackNofHitsMap.find(hit1->GetTrackID());
        if ( trackFromMap != trackNofHitsMap.end() ) {
            trackFromMap->second = trackFromMap->second+1;
        }
        else {
            trackNofHitsMap.insert(std::pair<int,int>{hit1->GetTrackID(),1});
        }
    }
    for ( int ihit2 = 0 ; ihit2 < nofHits2 ; ihit2++ ) {
        FairTutPropHit* hit2 = static_cast<FairTutPropHit*>(fHitsArray2->At(ihit2));
        auto trackFromMap = trackNofHitsMap.find(hit2->GetTrackID());
        if ( trackFromMap != trackNofHitsMap.end() ) {
            trackFromMap->second = trackFromMap->second+1;
        }
        else {
            trackNofHitsMap.insert(std::pair<int,int>{hit2->GetTrackID(),2});
        }
    }

    std::map<int, int>::iterator it;
    
    for ( it = trackNofHitsMap.begin(); it != trackNofHitsMap.end(); it++ ) {
        if ( it->second < 3 ) { // make a cut on the number of hits: 3
            continue;
        }

        bool trackCreated = false;
        for ( int ihit1 = 0 ; ihit1 < nofHits1 ; ihit1++ ) {
            FairTutPropHit* hit1 = static_cast<FairTutPropHit*>(fHitsArray1->At(ihit1));
            auto trackFromMap = trackNofHitsMap.find(hit1->GetTrackID());
            if ( trackFromMap != trackNofHitsMap.end() ) {
                FairTrackParP firstTrackPar (TVector3(hit1->GetX()  ,hit1->GetY()  ,hit1->GetZ()  ),
                                             TVector3(hit1->GetPx() ,hit1->GetPy() ,hit1->GetPz() ),
                                             TVector3(hit1->GetDx() ,hit1->GetDy() ,hit1->GetDz() ),
                                             TVector3(hit1->GetDPx(),hit1->GetDPy(),hit1->GetDPz()),
                                             hit1->GetCharge(),
                                             TVector3(0., 0., hit1->GetZ()),
                                             TVector3(1., 0., 0.),
                                             TVector3(0., 1., 0.));
                new ((*fTracksArray)[fTracksArray->GetEntries()]) FairTutPropTrack(hit1->GetPdgCode(),firstTrackPar);
                trackCreated = true;
                break;
            }
        }
        if ( trackCreated ) continue;
        for ( int ihit2 = 0 ; ihit2 < nofHits2 ; ihit2++ ) {
            FairTutPropHit* hit2 = static_cast<FairTutPropHit*>(fHitsArray2->At(ihit2));
            auto trackFromMap = trackNofHitsMap.find(hit2->GetTrackID());
            if ( trackFromMap != trackNofHitsMap.end() ) {
                FairTrackParP firstTrackPar (TVector3(hit2->GetX()  ,hit2->GetY()  ,hit2->GetZ()  ),
                                             TVector3(hit2->GetPx() ,hit2->GetPy() ,hit2->GetPz() ),
                                             TVector3(hit2->GetDx() ,hit2->GetDy() ,hit2->GetDz() ),
                                             TVector3(hit2->GetDPx(),hit2->GetDPy(),hit2->GetDPz()),
                                             hit2->GetCharge(),
                                             TVector3(0., 0., hit2->GetZ()),
                                             TVector3(1., 0., 0.),
                                             TVector3(0., 1., 0.));
                new ((*fTracksArray)[fTracksArray->GetEntries()]) FairTutPropTrack(hit2->GetPdgCode(),firstTrackPar);
                trackCreated = true;
                break;
            }
        }
        
    }
}

void FairTutPropTrackFinder::Finish() {
    LOG(debug) << "Finish of FairTutPropTrackFinder";
}

ClassImp(FairTutPropTrackFinder);
