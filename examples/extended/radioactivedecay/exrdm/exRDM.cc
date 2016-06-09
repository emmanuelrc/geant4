//
// ********************************************************************
// * DISCLAIMER                                                       *
// *                                                                  *
// * The following disclaimer summarizes all the specific disclaimers *
// * of contributors to this software. The specific disclaimers,which *
// * govern, are listed with their locations in:                      *
// *   http://cern.ch/geant4/license                                  *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.                                                             *
// *                                                                  *
// * This  code  implementation is the  intellectual property  of the *
// * GEANT4 collaboration.                                            *
// * By copying,  distributing  or modifying the Program (or any work *
// * based  on  the Program)  you indicate  your  acceptance of  this *
// * statement, and all its terms.                                    *
// ********************************************************************
//
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#ifdef G4UI_USE_GAG
#include "G4UIGAG.hh"
#endif
#include "G4UIterminal.hh"
#include "G4UItcsh.hh"
#ifdef G4UI_USE_XM
#include "G4UIXm.hh"
#endif 


#include "exrdmDetectorConstruction.hh"
#include "exrdmPhysicsList.hh"
#include "exrdmEventAction.hh"
#include "exrdmRunAction.hh"
#include "exrdmSteppingAction.hh"
#include "exrdmPrimaryGeneratorAction.hh"
#include "exrdmAnalysisManager.hh"
#include "Randomize.hh"

#ifdef G4VIS_USE
#include "exrdmVisManager.hh"
#endif

int main(int argc,char** argv)
{
  // random engine
  HepRandom::setTheEngine(new RanecuEngine);

  // Construct the default run manager
  G4RunManager* runManager = new G4RunManager;

  // Creation of the analysis manager
  exrdmAnalysisManager* analysis = exrdmAnalysisManager::getInstance();

  // set mandatory initialization classes

  exrdmDetectorConstruction* Detector = new exrdmDetectorConstruction;
  runManager->SetUserInitialization(Detector);
  runManager->SetUserInitialization(new exrdmPhysicsList);

  // set mandatory user action class
  runManager->SetUserAction(new exrdmPrimaryGeneratorAction);
  runManager->SetUserAction(new exrdmRunAction);
  runManager->SetUserAction(new exrdmEventAction);
  runManager->SetUserAction(new exrdmSteppingAction);
  //
  //
  G4UIsession* session=0;
  
  if (argc==1)   // Define UI session for interactive mode.
    {
#ifdef G4UI_USE_XM
      session = new G4UIXm(argc,argv);
#else           
#ifdef G4UI_USE_TCSH
      session = new G4UIterminal(new G4UItcsh);      
#else
      session = new G4UIterminal();
#endif
#endif
    }
 
#ifdef G4VIS_USE
  // visualization manager
  G4VisManager* visManager = new exrdmVisManager;
  visManager->Initialize();
#endif

  // Initialize G4 kernel
  // do this at run time so the geometry/physics can be changed
  //  runManager->Initialize();

  // get the pointer to the User Interface manager 
  G4UImanager* UI = G4UImanager::GetUIpointer();  
  if (session)   // Define UI session for interactive mode.
    {
      session->SessionStart();
      delete session;
    }
  else           // Batch mode
    { 
      G4String command = "/control/execute ";
      G4String fileName = argv[1];
      UI->ApplyCommand(command+fileName);
    }
  
  // job termination
#ifdef G4VIS_USE
  delete visManager;
#endif
  delete analysis;
  delete runManager;

  return 0;
}







