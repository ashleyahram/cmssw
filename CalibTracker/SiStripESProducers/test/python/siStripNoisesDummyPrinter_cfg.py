# The following comments couldn't be translated into the new config version:

# upload to database 

#string timetype = "timestamp"    

import FWCore.ParameterSet.Config as cms

process = cms.Process("Reader")

# Use this to have also debug info (WARNING: the resulting file is > 200MB.
process.MessageLogger = cms.Service("MessageLogger",
    PedestalsReaderDebug = cms.untracked.PSet(
        threshold = cms.untracked.string('DEBUG')
    ),
    PedestalsReaderSummary = cms.untracked.PSet(
        threshold = cms.untracked.string('INFO')
    ),
    cerr = cms.untracked.PSet(
        enable = cms.untracked.bool(False)
    ),
    cout = cms.untracked.PSet(
        threshold = cms.untracked.string('INFO')
    ),
    debugModules = cms.untracked.vstring('*'),
    files = cms.untracked.PSet(
        NoisesReaderDebug = cms.untracked.PSet(

        ),
        NoisesReaderSummary = cms.untracked.PSet(

        )
    )
)

#process.MessageLogger = cms.Service("MessageLogger",
#    debugModules = cms.untracked.vstring(''),
#    NoisesReader = cms.untracked.PSet(
#        threshold = cms.untracked.string('INFO')
#    ),
#    cout = cms.untracked.PSet(
#        threshold = cms.untracked.string('INFO')
#    ),
#    destinations = cms.untracked.vstring('NoisesReader.log')
#)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)
process.source = cms.Source("EmptySource",
    numberEventsInRun = cms.untracked.uint32(1),
    firstRun = cms.untracked.uint32(100000)
)

process.poolDBESSource = cms.ESSource("PoolDBESSource",
   DBParameters = cms.PSet(
        messageLevel = cms.untracked.int32(2),
        authenticationPath = cms.untracked.string('/afs/cern.ch/cms/DB/conddb')
    ),
    connect = cms.string('sqlite_file:dbfile.db'),
#    connect = cms.string('oracle://cms_orcoff_prod/CMS_COND_21X_STRIP'),
    toGet = cms.VPSet(cms.PSet(
        record = cms.string('SiStripNoisesRcd'),
#        tag = cms.string('SiStripNoise_CRAFT_21X_v4_offline')
        tag = cms.string('SiStripNoises_Ideal_31X')
    ))
)

process.reader = cms.EDFilter("SiStripNoisesDummyPrinter")
                              
process.p1 = cms.Path(process.reader)


