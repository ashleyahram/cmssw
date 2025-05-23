import FWCore.ParameterSet.Config as cms

process = cms.Process("PROD")

process.load("Configuration.StandardSequences.SimulationRandomNumberGeneratorSeeds_cff")


#process.source = cms.Source("EmptySource")
process.source = cms.Source("LHESource",
    fileNames = cms.untracked.vstring('file:ttbar.lhe')
)

process.generator = cms.EDFilter("Pythia8HepMC3HadronizerFilter",
    maxEventsToPrint = cms.untracked.int32(1),
    pythiaPylistVerbosity = cms.untracked.int32(1),
    filterEfficiency = cms.untracked.double(1.0),
    pythiaHepMCVerbosity = cms.untracked.bool(True),
    comEnergy = cms.double(7000.),
    #LHEInputFileName = cms.untracked.string('ttbar.lhe'),
    PythiaParameters = cms.PSet(
        pythia8_example03 = cms.vstring(''),
        parameterSets = cms.vstring('pythia8_example03')
    )
)

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger = cms.Service("MessageLogger",
    cerr = cms.untracked.PSet(
        enable = cms.untracked.bool(False)
    ),
    cout = cms.untracked.PSet(
        default = cms.untracked.PSet(
            limit = cms.untracked.int32(2)
        ),
        enable = cms.untracked.bool(True)
    )
)

process.RandomNumberGeneratorService = cms.Service("RandomNumberGeneratorService",
    generator = cms.PSet(
        initialSeed = cms.untracked.uint32(123456789),
    )
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(10)
)

process.GEN = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('pythia8hmc3ex3.root')
)

process.load("FWCore.Modules.printContent_cfi")

process.p = cms.Path(process.generator*process.printContent)
process.outpath = cms.EndPath(process.GEN)

process.schedule = cms.Schedule(process.p, process.outpath)
