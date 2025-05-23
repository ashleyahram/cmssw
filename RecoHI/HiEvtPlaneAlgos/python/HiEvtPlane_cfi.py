import FWCore.ParameterSet.Config as cms

hiEvtPlane = cms.EDProducer("EvtPlaneProducer",
                            centralityVariable = cms.string("HFtowers"),
                            centralityBinTag = cms.InputTag("centralityBin","HFtowers"),
                            vertexTag = cms.InputTag("hiSelectedVertex"),
                            caloTag = cms.InputTag("towerMaker"),
                            castorTag = cms.InputTag("CastorTowerReco"),
                            trackTag = cms.InputTag("hiGeneralTracks"),
                            lostTag = cms.InputTag("lostTracks"),
                            chi2MapTag = cms.InputTag("packedPFCandidateTrackChi2"),
                            chi2MapLostTag = cms.InputTag("lostTrackChi2"),
                            nonDefaultGlauberModel = cms.string(""),
                            loadDB = cms.bool(False),
                            minet = cms.double(-1.),
                            maxet = cms.double(-1),
                            minpt = cms.double(0.3),
                            maxpt = cms.double(3.0),
                            flatnvtxbins = cms.int32(10),
                            flatminvtx = cms.double(-15.0),
                            flatdelvtx = cms.double(3.0),
                            dzdzerror = cms.double(3.0),
                            d0d0error = cms.double(3.0),
                            pterror = cms.double(0.1),
                            chi2perlayer = cms.double(0.18),
                            dzdzerror_pix = cms.double(10.),
                            chi2 = cms.double(40.),
                            nhitsValid = cms.int32(11),
                            FlatOrder = cms.int32(9),
                            NumFlatBins = cms.int32(40),
                            caloCentRef = cms.double(80.),
                            caloCentRefWidth = cms.double(5.0),
                            CentBinCompression = cms.int32(5),
                            cutEra = cms.int32(2) # 0:ppReco, 1:HIReco, 2:Pixel, 3: GenMC
                            )

from Configuration.ProcessModifiers.pp_on_AA_cff import pp_on_AA
from Configuration.Eras.Modifier_run3_oxygen_cff import run3_oxygen
(pp_on_AA | run3_oxygen).toModify(hiEvtPlane,
    vertexTag = "offlinePrimaryVertices",
    trackTag = "packedPFCandidates",
    caloTag = "particleFlow",
    minet = 0.01,
    minpt = 0.5,
    dzdzerror_pix = 40.,
    caloCentRef = -1,
    caloCentRefWidth = -1,
    cutEra = 0
)
