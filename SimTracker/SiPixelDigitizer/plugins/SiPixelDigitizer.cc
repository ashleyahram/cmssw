// -*- C++ -*-
//
// Package:    SiPixelDigitizer
// Class:      SiPixelDigitizer
//
/**\class SiPixelDigitizer SiPixelDigitizer.cc SimTracker/SiPixelDigitizer/src/SiPixelDigitizer.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Michele Pioppi-INFN perugia
//   Modifications: Freya Blekman - Cornell University
//         Created:  Mon Sep 26 11:08:32 CEST 2005
//
//

// system include files
#include <memory>
#include <set>

// user include files
#include "SiPixelDigitizer.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelSimHitExtraInfo.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelSimHitExtraInfoLite.h"
#include "PixelDigiAddTempInfo.h"
#include "SiPixelDigitizerAlgorithm.h"

#include "SimDataFormats/TrackingHit/interface/PSimHit.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "SimDataFormats/TrackerDigiSimLink/interface/PixelDigiSimLink.h"
#include "DataFormats/Common/interface/DetSet.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigiCollection.h"
#include "SimDataFormats/TrackingHit/interface/PSimHitContainer.h"
#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/GeometryVector/interface/LocalVector.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetType.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "MagneticField/Engine/interface/MagneticField.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "SimGeneral/MixingModule/interface/PileUpEventPrincipal.h"
#include "DataFormats/SiPixelDetId/interface/PixelFEDChannel.h"
//Random Number
#include "FWCore/AbstractServices/interface/RandomNumberGenerator.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Utilities/interface/Exception.h"

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
//using namespace std;

namespace cms {
  SiPixelDigitizer::SiPixelDigitizer(const edm::ParameterSet& iConfig,
                                     edm::ProducesCollector producesCollector,
                                     edm::ConsumesCollector& iC)
      : firstInitializeEvent_(true),
        firstFinalizeEvent_(true),
        applyLateReweighting_(
            iConfig.exists("applyLateReweighting") ? iConfig.getParameter<bool>("applyLateReweighting") : false),
        usePixelExtraLiteFormat_(
            iConfig.exists("usePixelExtraLiteFormat") ? iConfig.getParameter<bool>("usePixelExtraLiteFormat") : false),
        store_SimHitEntryExitPoints_(iConfig.exists("store_SimHitEntryExitPoints")
                                         ? iConfig.getParameter<bool>("store_SimHitEntryExitPoints")
                                         : false),
        store_SimHitEntryExitPointsLite_(iConfig.exists("store_SimHitEntryExitPointsLite")
                                             ? iConfig.getParameter<bool>("store_SimHitEntryExitPointsLite")
                                             : false),
        _pixeldigialgo(),
        hitsProducer(iConfig.getParameter<std::string>("hitsProducer")),
        trackerContainers(iConfig.getParameter<std::vector<std::string> >("RoutList")),
        pilotBlades(iConfig.exists("enablePilotBlades") ? iConfig.getParameter<bool>("enablePilotBlades") : false),
        NumberOfEndcapDisks(iConfig.exists("NumPixelEndcap") ? iConfig.getParameter<int>("NumPixelEndcap") : 2),
        tTopoToken_(iC.esConsumes()),
        pDDToken_(iC.esConsumes(edm::ESInputTag("", iConfig.getParameter<std::string>("PixGeometryType")))),
        pSetupToken_(iC.esConsumes()) {
    edm::LogInfo("PixelDigitizer ") << "Enter the Pixel Digitizer";

    edm::LogInfo("PixelDigitizer ") << " applyLateReweighting_ " << applyLateReweighting_;

    const std::string alias("simSiPixelDigis");

    producesCollector.produces<edm::DetSetVector<PixelDigi> >().setBranchAlias(alias);
    producesCollector.produces<edm::DetSetVector<PixelDigiSimLink> >().setBranchAlias(alias + "siPixelDigiSimLink");
    if (store_SimHitEntryExitPoints_)
      producesCollector.produces<edm::DetSetVector<PixelSimHitExtraInfo> >().setBranchAlias(alias +
                                                                                            "siPixelExtraSimHit");
    if (store_SimHitEntryExitPointsLite_)
      producesCollector.produces<edm::DetSetVector<PixelSimHitExtraInfoLite> >().setBranchAlias(
          alias + "siPixelExtraSimHitLite");

    for (auto const& trackerContainer : trackerContainers) {
      edm::InputTag tag(hitsProducer, trackerContainer);
      iC.consumes<std::vector<PSimHit> >(edm::InputTag(hitsProducer, trackerContainer));
    }
    edm::Service<edm::RandomNumberGenerator> rng;
    if (!rng.isAvailable()) {
      throw cms::Exception("Configuration")
          << "SiPixelDigitizer requires the RandomNumberGeneratorService\n"
             "which is not present in the configuration file.  You must add the service\n"
             "in the configuration file or remove the modules that require it.";
    }

    _pixeldigialgo = std::make_unique<SiPixelDigitizerAlgorithm>(iConfig, iC);
    if (NumberOfEndcapDisks != 2)
      producesCollector.produces<PixelFEDChannelCollection>();
  }

  SiPixelDigitizer::~SiPixelDigitizer() { edm::LogInfo("PixelDigitizer ") << "Destruct the Pixel Digitizer"; }

  //
  // member functions
  //

  void SiPixelDigitizer::accumulatePixelHits(edm::Handle<std::vector<PSimHit> > hSimHits,
                                             size_t globalSimHitIndex,
                                             const unsigned int tofBin,
                                             edm::EventSetup const& iSetup) {
    if (hSimHits.isValid()) {
      std::set<unsigned int> detIds;
      std::vector<PSimHit> const& simHits = *hSimHits.product();
      const TrackerTopology* tTopo = &iSetup.getData(tTopoToken_);
      _pixeldigialgo->fillSimHitMaps(simHits, tofBin);
      for (std::vector<PSimHit>::const_iterator it = simHits.begin(), itEnd = simHits.end(); it != itEnd;
           ++it, ++globalSimHitIndex) {
        unsigned int detId = (*it).detUnitId();
        if (detIds.insert(detId).second) {
          // The insert succeeded, so this detector element has not yet been processed.
          assert(detectorUnits[detId]);
          if (detectorUnits[detId] &&
              detectorUnits[detId]
                  ->type()
                  .isTrackerPixel()) {  // this test could be avoided and changed into a check of pixdet!=0
            std::map<unsigned int, PixelGeomDetUnit const*>::iterator itDet = detectorUnits.find(detId);
            if (itDet == detectorUnits.end())
              continue;
            auto pixdet = itDet->second;
            assert(pixdet != nullptr);
            //access to magnetic field in global coordinates
            GlobalVector bfield = pSetup->inTesla(pixdet->surface().position());
            LogDebug("PixelDigitizer ") << "B-field(T) at " << pixdet->surface().position()
                                        << "(cm): " << pSetup->inTesla(pixdet->surface().position());
            _pixeldigialgo->accumulateSimHits(
                it, itEnd, globalSimHitIndex, tofBin, pixdet, bfield, tTopo, randomEngine_);
          }
        }
      }
    }
  }

  void SiPixelDigitizer::initializeEvent(edm::Event const& e, edm::EventSetup const& iSetup) {
    if (firstInitializeEvent_) {
      _pixeldigialgo->init(iSetup);
      firstInitializeEvent_ = false;
    }

    // Make sure that the first crossing processed starts indexing the sim hits from zero.
    // This variable is used so that the sim hits from all crossing frames have sequential
    // indices used to create the digi-sim link (if configured to do so) rather than starting
    // from zero for each crossing.
    crossingSimHitIndexOffset_.clear();

    // Cache random number engine
    edm::Service<edm::RandomNumberGenerator> rng;
    randomEngine_ = &rng->getEngine(e.streamID());

    _pixeldigialgo->initializeEvent();
    pDD = &iSetup.getData(pDDToken_);
    pSetup = &iSetup.getData(pSetupToken_);
    const TrackerTopology* tTopo = &iSetup.getData(tTopoToken_);

    // FIX THIS! We only need to clear and (re)fill this map when the geometry type IOV changes.  Use ESWatcher to determine this.
    if (true) {  // Replace with ESWatcher
      detectorUnits.clear();
      for (const auto& iu : pDD->detUnits()) {
        unsigned int detId = iu->geographicalId().rawId();
        if (iu->type().isTrackerPixel()) {
          auto pixdet = dynamic_cast<const PixelGeomDetUnit*>(iu);
          assert(pixdet != nullptr);
          if (iu->subDetector() ==
              GeomDetEnumerators::SubDetector::PixelEndcap) {  // true ONLY for the phase 0 pixel deetctor
            unsigned int disk = tTopo->layer(detId);           // using the generic layer method
            //if using pilot blades, then allowing it for current detector only
            if ((disk == 3) && ((!pilotBlades) && (NumberOfEndcapDisks == 2)))
              continue;
          }
          detectorUnits.insert(std::make_pair(detId, pixdet));
        }
      }
    }
  }

  void SiPixelDigitizer::accumulate(edm::Event const& iEvent, edm::EventSetup const& iSetup) {
    // Step A: Get Inputs
    for (vstring::const_iterator i = trackerContainers.begin(), iEnd = trackerContainers.end(); i != iEnd; ++i) {
      edm::Handle<std::vector<PSimHit> > simHits;
      edm::InputTag tag(hitsProducer, *i);

      iEvent.getByLabel(tag, simHits);
      unsigned int tofBin = PixelDigiSimLink::LowTof;
      if ((*i).find(std::string("HighTof")) != std::string::npos)
        tofBin = PixelDigiSimLink::HighTof;
      accumulatePixelHits(simHits, crossingSimHitIndexOffset_[tag.encode()], tofBin, iSetup);
      // Now that the hits have been processed, I'll add the amount of hits in this crossing on to
      // the global counter. Next time accumulateStripHits() is called it will count the sim hits
      // as though they were on the end of this collection.
      // Note that this is only used for creating digi-sim links (if configured to do so).
      //       std::cout << "index offset, current hit count = " << crossingSimHitIndexOffset_[tag.encode()] << ", " << simHits->size() << std::endl;
      if (simHits.isValid())
        crossingSimHitIndexOffset_[tag.encode()] += simHits->size();
    }
  }

  void SiPixelDigitizer::accumulate(PileUpEventPrincipal const& iEvent,
                                    edm::EventSetup const& iSetup,
                                    edm::StreamID const& streamID) {
    // Step A: Get Inputs
    for (vstring::const_iterator i = trackerContainers.begin(), iEnd = trackerContainers.end(); i != iEnd; ++i) {
      edm::Handle<std::vector<PSimHit> > simHits;
      edm::InputTag tag(hitsProducer, *i);

      iEvent.getByLabel(tag, simHits);
      unsigned int tofBin = PixelDigiSimLink::LowTof;
      if ((*i).find(std::string("HighTof")) != std::string::npos)
        tofBin = PixelDigiSimLink::HighTof;
      accumulatePixelHits(simHits, crossingSimHitIndexOffset_[tag.encode()], tofBin, iSetup);
      // Now that the hits have been processed, I'll add the amount of hits in this crossing on to
      // the global counter. Next time accumulateStripHits() is called it will count the sim hits
      // as though they were on the end of this collection.
      // Note that this is only used for creating digi-sim links (if configured to do so).
      //       std::cout << "index offset, current hit count = " << crossingSimHitIndexOffset_[tag.encode()] << ", " << simHits->size() << std::endl;
      if (simHits.isValid())
        crossingSimHitIndexOffset_[tag.encode()] += simHits->size();
    }
  }

  // ------------ method called to produce the data  ------------
  void SiPixelDigitizer::finalizeEvent(edm::Event& iEvent, const edm::EventSetup& iSetup) {
    const TrackerTopology* tTopo = &iSetup.getData(tTopoToken_);

    std::vector<edm::DetSet<PixelDigi> > theDigiVector;
    std::vector<edm::DetSet<PixelDigiSimLink> > theDigiLinkVector;
    std::vector<edm::DetSet<PixelSimHitExtraInfo> > theExtraSimHitInfoVector;
    std::vector<edm::DetSet<PixelSimHitExtraInfoLite> > theExtraSimHitInfoLiteVector;

    if (firstFinalizeEvent_) {
      _pixeldigialgo->init_DynIneffDB(iSetup);
      firstFinalizeEvent_ = false;
    }
    _pixeldigialgo->calculateInstlumiFactor(PileupInfo_.get());

    if (_pixeldigialgo->killBadFEDChannels()) {
      std::unique_ptr<PixelFEDChannelCollection> PixelFEDChannelCollection_ =
          _pixeldigialgo->chooseScenario(PileupInfo_.get(), randomEngine_);
      if (PixelFEDChannelCollection_ == nullptr) {
        throw cms::Exception("NullPointerError") << "PixelFEDChannelCollection not set in chooseScenario function.\n";
      }
      iEvent.put(std::move(PixelFEDChannelCollection_));
    }

    for (const auto& iu : pDD->detUnits()) {
      if (iu->type().isTrackerPixel()) {
        //

        edm::DetSet<PixelDigi> collector(iu->geographicalId().rawId());
        edm::DetSet<PixelDigiSimLink> linkcollector(iu->geographicalId().rawId());
        std::vector<PixelDigiAddTempInfo> tempcollector;
        edm::DetSet<PixelSimHitExtraInfo> tempSHcollector(iu->geographicalId().rawId());
        edm::DetSet<PixelSimHitExtraInfoLite> tempSHLitecollector(iu->geographicalId().rawId());

        _pixeldigialgo->digitize(dynamic_cast<const PixelGeomDetUnit*>(iu),
                                 collector.data,
                                 linkcollector.data,
                                 tempcollector,
                                 tTopo,
                                 randomEngine_);

        // transformation of the tempcollector  (October 15, 2021)
        if (!tempcollector.empty()) {
          std::vector<PixelDigiAddTempInfo>::const_iterator loopNewClass;
          unsigned int channelPrevious2 = -1;
          size_t hitFirstOne2 = -1;
          for (loopNewClass = tempcollector.begin(); loopNewClass != tempcollector.end(); ++loopNewClass) {
            //   check if the new SimHit already exists in the class
            //      if yes : add only the Digi info to the existing entry
            //      if not : create a new entry
            //          PixelSimHitExtraInfo(
            //          size_t Hindex, Local3DPoint entryP , Local3DPoint exitP, uint32_t detID, std::vector<unsigned int> ch, std::vector<float> InitCharge)
            //   what about the duplicates : 2 SimHits associated to the same Digis ?
            //
            //

            bool checkTwoSimHits = false;
            if (channelPrevious2 == loopNewClass->channel() && hitFirstOne2 != loopNewClass->hitIndex()) {
              // case of one Digi associated to a second SimHit
              checkTwoSimHits = true;
            } else {
              channelPrevious2 = loopNewClass->channel();
              hitFirstOne2 = loopNewClass->hitIndex();
            }

            bool checkInTheList = false;
            // To fill the PixelSimHitExtraInfo temporary collector
            if (!checkTwoSimHits) {
              std::vector<PixelSimHitExtraInfo>::iterator loopTempSH;
              for (loopTempSH = tempSHcollector.begin(); loopTempSH != tempSHcollector.end(); ++loopTempSH) {
                if (loopNewClass->hitIndex() == loopTempSH->hitIndex()) {
                  checkInTheList = true;
                  loopTempSH->addDigiInfo(loopNewClass->channel());
                }
              }
              if (!checkInTheList) {
                PixelSimHitExtraInfo newSHEntry(loopNewClass->hitIndex(),
                                                loopNewClass->entryPoint(),
                                                loopNewClass->exitPoint(),
                                                loopNewClass->channel());
                tempSHcollector.push_back(newSHEntry);
              }
            }
            bool checkInTheListLite = false;
            // To fill the PixelSimHitExtraInfoLite temporary collector
            if (!checkTwoSimHits) {
              std::vector<PixelSimHitExtraInfoLite>::iterator loopTempSHLite;
              for (loopTempSHLite = tempSHLitecollector.begin(); loopTempSHLite != tempSHLitecollector.end();
                   ++loopTempSHLite) {
                if (loopNewClass->hitIndex() == loopTempSHLite->hitIndex()) {
                  checkInTheListLite = true;
                  loopTempSHLite->addDigiInfo(loopNewClass->channel());
                }
              }
              if (!checkInTheListLite) {
                PixelSimHitExtraInfoLite newSHLiteEntry(loopNewClass->hitIndex(),
                                                        loopNewClass->entryPoint(),
                                                        loopNewClass->exitPoint(),
                                                        loopNewClass->channel());
                tempSHLitecollector.push_back(newSHLiteEntry);
              }
            }
          }
        }

        if (applyLateReweighting_) {
          if (!usePixelExtraLiteFormat_) {
            // if applyLateReweighting_  is true, the charge reweighting has to be applied on top of the digis
            _pixeldigialgo->lateSignalReweight(
                dynamic_cast<const PixelGeomDetUnit*>(iu), collector.data, tempSHcollector.data, tTopo, randomEngine_);
          } else {
            // if applyLateReweighting_  is true, the charge reweighting has to be applied on top of the digis
            _pixeldigialgo->lateSignalReweight(dynamic_cast<const PixelGeomDetUnit*>(iu),
                                               collector.data,
                                               tempSHLitecollector.data,
                                               tTopo,
                                               randomEngine_);
          }
        }

        if (!collector.data.empty()) {
          theDigiVector.push_back(std::move(collector));
        }
        if (!linkcollector.data.empty()) {
          theDigiLinkVector.push_back(std::move(linkcollector));
        }
        if (!tempSHcollector.data.empty()) {
          theExtraSimHitInfoVector.push_back(std::move(tempSHcollector));
        }
        if (!tempSHLitecollector.data.empty()) {
          theExtraSimHitInfoLiteVector.push_back(std::move(tempSHLitecollector));
        }
      }
    }
    _pixeldigialgo->resetSimHitMaps();

    // Step C: create collection with the cache vector of DetSet
    std::unique_ptr<edm::DetSetVector<PixelDigi> > output(new edm::DetSetVector<PixelDigi>(theDigiVector));
    std::unique_ptr<edm::DetSetVector<PixelDigiSimLink> > outputlink(
        new edm::DetSetVector<PixelDigiSimLink>(theDigiLinkVector));
    std::unique_ptr<edm::DetSetVector<PixelSimHitExtraInfo> > outputExtraSim(
        new edm::DetSetVector<PixelSimHitExtraInfo>(theExtraSimHitInfoVector));
    std::unique_ptr<edm::DetSetVector<PixelSimHitExtraInfoLite> > outputExtraSimLite(
        new edm::DetSetVector<PixelSimHitExtraInfoLite>(theExtraSimHitInfoLiteVector));

    // Step D: write output to file
    iEvent.put(std::move(output));
    iEvent.put(std::move(outputlink));
    if (store_SimHitEntryExitPoints_)
      iEvent.put(std::move(outputExtraSim));
    if (store_SimHitEntryExitPointsLite_)
      iEvent.put(std::move(outputExtraSimLite));

    randomEngine_ = nullptr;  // to prevent access outside event
  }
}  // namespace cms
