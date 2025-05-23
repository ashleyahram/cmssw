import FWCore.ParameterSet.Config as cms
from PhysicsTools.PatAlgos.slimming.prunedGenParticles_cfi import *
from PhysicsTools.PatAlgos.slimming.packedGenParticles_cfi import *

prunedGenParticlesWithStatusOne = prunedGenParticles.clone()
prunedGenParticlesWithStatusOne.select.append( "keep    status == 1")

prunedGenParticles.src =  cms.InputTag("prunedGenParticlesWithStatusOne")

packedGenParticles.inputCollection = cms.InputTag("prunedGenParticlesWithStatusOne")
packedGenParticles.map = cms.InputTag("prunedGenParticles") # map with rekey association from prunedGenParticlesWithStatusOne to prunedGenParticles, used to relink our refs to prunedGen
packedGenParticles.inputOriginal = cms.InputTag("genParticles")

genParticlesTask = cms.Task(
    prunedGenParticles,
    packedGenParticles,
    prunedGenParticlesWithStatusOne
)

from PhysicsTools.PatAlgos.packedGenParticlesSignal_cfi import *

_genParticlesHITask = genParticlesTask.copy()
_genParticlesHITask.add(packedGenParticlesSignal)

from Configuration.ProcessModifiers.pp_on_AA_cff import pp_on_AA
from Configuration.Eras.Modifier_run3_oxygen_cff import run3_oxygen
(pp_on_AA | run3_oxygen).toReplaceWith(genParticlesTask, _genParticlesHITask)
