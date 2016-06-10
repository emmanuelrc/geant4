//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// INCL++ intra-nuclear cascade model
// Pekka Kaitaniemi, CEA and Helsinki Institute of Physics
// Davide Mancusi, CEA
// Alain Boudard, CEA
// Sylvie Leray, CEA
// Joseph Cugnon, University of Liege
//
#define INCLXX_IN_GEANT4_MODE 1

#include "globals.hh"

#include "G4INCLNuclearDensity.hh"
#include "G4INCLParticleTable.hh"
#include "G4INCLGlobals.hh"
#include <algorithm>

namespace G4INCL {

  NuclearDensity::NuclearDensity(const G4int A, const G4int Z, InverseInterpolationTable const * const rpCorrelationTableProton, InverseInterpolationTable const * const rpCorrelationTableNeutron) :
    theA(A),
    theZ(Z),
    theMaximumRadius(std::min((*rpCorrelationTableProton)(1.), (*rpCorrelationTableNeutron)(1.))),
    theProtonNuclearRadius(ParticleTable::getNuclearRadius(Proton,theA,theZ))
  {
    std::fill(rFromP, rFromP + UnknownParticle, static_cast<InverseInterpolationTable*>(NULL));
    rFromP[Proton] = rpCorrelationTableProton;
    rFromP[Neutron] = rpCorrelationTableNeutron;
    rFromP[DeltaPlusPlus] = rpCorrelationTableProton;
    rFromP[DeltaPlus] = rpCorrelationTableProton;
    rFromP[DeltaZero] = rpCorrelationTableNeutron;
    rFromP[DeltaMinus] = rpCorrelationTableNeutron;
    // The interpolation table for local-energy look-ups is simply obtained by
    // inverting the r-p correlation table.
    std::fill(pFromR, pFromR + UnknownParticle, static_cast<InverseInterpolationTable*>(NULL));
    pFromR[Proton] = new InverseInterpolationTable(rFromP[Proton]->getNodeValues(), rFromP[Proton]->getNodeAbscissae());
    pFromR[Neutron] = new InverseInterpolationTable(rFromP[Neutron]->getNodeValues(), rFromP[Neutron]->getNodeAbscissae());
    pFromR[DeltaPlusPlus] = new InverseInterpolationTable(rFromP[DeltaPlusPlus]->getNodeValues(), rFromP[DeltaPlusPlus]->getNodeAbscissae());
    pFromR[DeltaPlus] = new InverseInterpolationTable(rFromP[DeltaPlus]->getNodeValues(), rFromP[DeltaPlus]->getNodeAbscissae());
    pFromR[DeltaZero] = new InverseInterpolationTable(rFromP[DeltaZero]->getNodeValues(), rFromP[DeltaZero]->getNodeAbscissae());
    pFromR[DeltaMinus] = new InverseInterpolationTable(rFromP[DeltaMinus]->getNodeValues(), rFromP[DeltaMinus]->getNodeAbscissae());
    INCL_DEBUG("Interpolation table for proton local energy (A=" << theA << ", Z=" << theZ << ") initialised:"
          << std::endl
          << pFromR[Proton]->print()
          << std::endl
          << "Interpolation table for neutron local energy (A=" << theA << ", Z=" << theZ << ") initialised:"
          << std::endl
          << pFromR[Neutron]->print()
          << std::endl
          << "Interpolation table for delta++ local energy (A=" << theA << ", Z=" << theZ << ") initialised:"
          << std::endl
          << pFromR[DeltaPlusPlus]->print()
          << std::endl
          << "Interpolation table for delta+ local energy (A=" << theA << ", Z=" << theZ << ") initialised:"
          << std::endl
          << pFromR[DeltaPlus]->print()
          << std::endl
          << "Interpolation table for delta0 local energy (A=" << theA << ", Z=" << theZ << ") initialised:"
          << std::endl
          << pFromR[DeltaZero]->print()
          << std::endl
          << "Interpolation table for delta- local energy (A=" << theA << ", Z=" << theZ << ") initialised:"
          << std::endl
          << pFromR[DeltaMinus]->print()
          << std::endl);
    initializeTransmissionRadii();
  }

  NuclearDensity::~NuclearDensity() {
    // We don't delete the rFromP tables, which are cached in the
    // NuclearDensityFactory
    delete pFromR[Proton];
    delete pFromR[Neutron];
    delete pFromR[DeltaPlusPlus];
    delete pFromR[DeltaPlus];
    delete pFromR[DeltaZero];
    delete pFromR[DeltaMinus];
  }

  NuclearDensity::NuclearDensity(const NuclearDensity &rhs) :
    theA(rhs.theA),
    theZ(rhs.theZ),
    theMaximumRadius(rhs.theMaximumRadius),
    theProtonNuclearRadius(rhs.theProtonNuclearRadius)
  {
    // rFromP is owned by NuclearDensityFactory, so shallow copy is sufficient
    std::fill(rFromP, rFromP + UnknownParticle, static_cast<InverseInterpolationTable*>(NULL));
    rFromP[Proton] = rhs.rFromP[Proton];
    rFromP[Neutron] = rhs.rFromP[Neutron];
    rFromP[DeltaPlusPlus] = rhs.rFromP[DeltaPlusPlus];
    rFromP[DeltaPlus] = rhs.rFromP[DeltaPlus];
    rFromP[DeltaZero] = rhs.rFromP[DeltaZero];
    rFromP[DeltaMinus] = rhs.rFromP[DeltaMinus];
    // deep copy for pFromR
    std::fill(pFromR, pFromR + UnknownParticle, static_cast<InverseInterpolationTable*>(NULL));
    pFromR[Proton] = new InverseInterpolationTable(*(rhs.pFromR[Proton]));
    pFromR[Neutron] = new InverseInterpolationTable(*(rhs.pFromR[Neutron]));
    pFromR[DeltaPlusPlus] = new InverseInterpolationTable(*(rhs.pFromR[DeltaPlusPlus]));
    pFromR[DeltaPlus] = new InverseInterpolationTable(*(rhs.pFromR[DeltaPlus]));
    pFromR[DeltaZero] = new InverseInterpolationTable(*(rhs.pFromR[DeltaZero]));
    pFromR[DeltaMinus] = new InverseInterpolationTable(*(rhs.pFromR[DeltaMinus]));
    std::copy(rhs.transmissionRadius, rhs.transmissionRadius+UnknownParticle, transmissionRadius);
  }

  NuclearDensity &NuclearDensity::operator=(const NuclearDensity &rhs) {
    NuclearDensity temporaryDensity(rhs);
    swap(temporaryDensity);
    return *this;
  }

  void NuclearDensity::swap(NuclearDensity &rhs) {
    std::swap(theA, rhs.theA);
    std::swap(theZ, rhs.theZ);
    std::swap(theMaximumRadius, rhs.theMaximumRadius);
    std::swap(theProtonNuclearRadius, rhs.theProtonNuclearRadius);
    std::swap_ranges(transmissionRadius, transmissionRadius+UnknownParticle, rhs.transmissionRadius);
    std::swap(rFromP[Proton], rhs.rFromP[Proton]);
    std::swap(rFromP[Neutron], rhs.rFromP[Neutron]);
    std::swap(rFromP[DeltaPlusPlus], rhs.rFromP[DeltaPlusPlus]);
    std::swap(rFromP[DeltaPlus], rhs.rFromP[DeltaPlus]);
    std::swap(rFromP[DeltaZero], rhs.rFromP[DeltaZero]);
    std::swap(rFromP[DeltaMinus], rhs.rFromP[DeltaMinus]);
    std::swap(pFromR[Proton], rhs.pFromR[Proton]);
    std::swap(pFromR[Neutron], rhs.pFromR[Neutron]);
    std::swap(pFromR[DeltaPlusPlus], rhs.pFromR[DeltaPlusPlus]);
    std::swap(pFromR[DeltaPlus], rhs.pFromR[DeltaPlus]);
    std::swap(pFromR[DeltaZero], rhs.pFromR[DeltaZero]);
    std::swap(pFromR[DeltaMinus], rhs.pFromR[DeltaMinus]);
 }

  void NuclearDensity::initializeTransmissionRadii() {
    const G4double theProtonRadius = 0.88; // fm
    const G4double theProtonTransmissionRadius = theProtonNuclearRadius + theProtonRadius;

    transmissionRadius[Proton] = theProtonTransmissionRadius;
    transmissionRadius[PiPlus] = theProtonNuclearRadius;
    transmissionRadius[PiMinus] = theProtonNuclearRadius;
    transmissionRadius[DeltaPlusPlus] = theProtonTransmissionRadius;
    transmissionRadius[DeltaPlus] = theProtonTransmissionRadius;
    transmissionRadius[DeltaMinus] = theProtonTransmissionRadius;
    transmissionRadius[Composite] = theProtonNuclearRadius;
    // transmission radii for neutral particles intentionally left uninitialised
  }

  G4double NuclearDensity::getMaxRFromP(ParticleType const t, const G4double p) const {
// assert(t==Proton || t==Neutron || t==DeltaPlusPlus || t==DeltaPlus || t==DeltaZero || t==DeltaMinus);
    return (*(rFromP[t]))(p);
  }

  G4double NuclearDensity::getMinPFromR(ParticleType const t, const G4double r) const {
// assert(t==Proton || t==Neutron || t==DeltaPlusPlus || t==DeltaPlus || t==DeltaZero || t==DeltaMinus);
    return (*(pFromR[t]))(r);
  }

}