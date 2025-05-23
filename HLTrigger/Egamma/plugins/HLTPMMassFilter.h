#ifndef HLTPMMassFilter_h
#define HLTPMMassFilter_h

/** \class HLTPMMassFilter
 *
 *  Original Author: Jeremy Werner
 *  Institution: Princeton University, USA
 *  Contact: Jeremy.Werner@cern.ch
 *  Date: February 21, 2007
 */

#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/EgammaCandidates/interface/Electron.h"
#include "DataFormats/EgammaCandidates/interface/ElectronFwd.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EgammaReco/interface/SuperClusterFwd.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "DataFormats/RecoCandidate/interface/RecoEcalCandidate.h"
#include "DataFormats/RecoCandidate/interface/RecoEcalCandidateFwd.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "HLTrigger/HLTcore/interface/HLTFilter.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "MagneticField/Engine/interface/MagneticField.h"
#include "TrackingTools/TrajectoryState/interface/ftsFromVertexToPoint.h"

#include "TLorentzVector.h"

class HLTPMMassFilter : public HLTFilter {
public:
  explicit HLTPMMassFilter(const edm::ParameterSet&);
  ~HLTPMMassFilter() override;
  bool hltFilter(edm::Event&,
                 const edm::EventSetup&,
                 trigger::TriggerFilterObjectWithRefs& filterproduct) const override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  bool isGoodPair(TLorentzVector const& v1, TLorentzVector const& v2) const;

  TLorentzVector approxMomAtVtx(const MagneticField& magField,
                                const GlobalPoint& xvert,
                                const reco::SuperClusterRef sc,
                                int charge) const;

  edm::InputTag candTag_;  // input tag identifying product contains filtered egammas
  edm::EDGetTokenT<trigger::TriggerFilterObjectWithRefs> candToken_;
  edm::InputTag beamSpot_;  // input tag identifying beamSpot product
  edm::EDGetTokenT<reco::BeamSpot> beamSpotToken_;
  double lowerMassCut_;
  double upperMassCut_;
  double lowerdRCut_;
  double upperdRCut_;
  double lowerdR2Cut_;
  double upperdR2Cut_;
  int nZcandcut_;  // number of Z candidates required
  bool reqOppCharge_;

  bool isElectron1_;
  bool isElectron2_;
  edm::InputTag l1EGTag_;

  edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> const magFieldToken_;
};

#endif  //HLTPMMassFilter_h
