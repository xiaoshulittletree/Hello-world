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
//
// $Id: XrayFluoAnalysisManager.cc
// GEANT4 tag $Name: xray_fluo-V03-02-00
//
// Author: Elena Guardincerri (Elena.Guardincerri@ge.infn.it)
//
// History:
// -----------
// 20 Jul 2007 A.Mantero, code cleaning and update. Replaced histos with tuple
// 11 Jul 2003 A.Mantero, code cleaning / Plotter-XML addiction
//    Sep 2002 A.Mantero, AIDA3.0 Migration
// 06 Dec 2001 A.Pfeiffer updated for singleton
// 30 Nov 2001 Guy Barrand : migrate to AIDA-2.2.
// 28 Nov 2001 Elena Guardincerri     Created
//
// -------------------------------------------------------------------
//#include "g4root.hh"
//#include "g4xml.hh" //single place to change format!
#include "g4csv.hh"
#include "G4VProcess.hh"
#include "XrayFluoAnalysisManager.hh"
#include "G4Step.hh"
#include "XrayFluoDetectorConstruction.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Gamma.hh"
#include "G4Electron.hh"
#include "G4Proton.hh"
#include "G4SystemOfUnits.hh"

#include "G4RunManager.hh"//Added to get event ID


XrayFluoAnalysisManager* XrayFluoAnalysisManager::instance = 0;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

namespace {
  //Mutex to acquire access to singleton instance check/creation
  G4Mutex instanceMutex = G4MUTEX_INITIALIZER;
  //Mutex to acquire accss to histograms creation/access
  //It is also used to control all operations related to histos
  //File writing and check analysis
  G4Mutex dataManipulationMutex = G4MUTEX_INITIALIZER;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

XrayFluoAnalysisManager::XrayFluoAnalysisManager()
  :outputFileName("xrayfluo"), phaseSpaceFlag(true), physicFlag (false),
   gunParticleEnergies(0), gunParticleTypes(0), photonnumber(0),
  electronnumber(0)
{
  dataLoaded = false;
  fParticleEnergyAndTypeIndex = 0;

  //creating the messenger
  analisysMessenger = new XrayFluoAnalysisMessenger(this);

  //Instantiate the analysis manager
  G4AnalysisManager::Instance();

  G4cout << "XrayFluoAnalysisManager created" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

XrayFluoAnalysisManager::~XrayFluoAnalysisManager()
{
  if ( gunParticleEnergies ) delete gunParticleEnergies;
  gunParticleEnergies = 0;
  if ( gunParticleTypes ) delete gunParticleTypes;
  gunParticleTypes = 0;

  delete instance;
  instance = 0;

  delete G4AnalysisManager::Instance();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

XrayFluoAnalysisManager* XrayFluoAnalysisManager::getInstance()

{
  G4AutoLock l(&instanceMutex);
  if (instance == 0) {instance = new XrayFluoAnalysisManager;}
  return instance;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void XrayFluoAnalysisManager::book()
{
  G4AutoLock l(&dataManipulationMutex);
  // Get analysis manager
  G4AnalysisManager* man = G4AnalysisManager::Instance();
  // Open an output file
  man->OpenFile(outputFileName);
  man->SetVerboseLevel(1);
  man->SetFirstHistoId(1);
  man->SetFirstNtupleId(1);

  G4cout << "Open output file: " << outputFileName << G4endl;

  if (phaseSpaceFlag)
    {
      // Book output Tuple (ID = 1)
      man->CreateNtuple("gamma","photon OutputNTuple");
      man->CreateNtupleIColumn("particle"); //int
      man->CreateNtupleIColumn("eventID");
      man->CreateNtupleDColumn("energies"); //double
    //  man->CreateNtupleDColumn("momentumTheta");
    //  man->CreateNtupleDColumn("momentumPhi");
      man->CreateNtupleDColumn("existPosTheta");
      man->CreateNtupleDColumn("existPosPhi");
      man->CreateNtupleDColumn("existPosRho");
      man->CreateNtupleDColumn("PrimaryMomentumTheta");


  //   man->CreateNtupleIColumn("processes");
  //    man->CreateNtupleIColumn("material");
  //    man->CreateNtupleIColumn("origin");
  //    man->CreateNtupleDColumn("depth");
      man->FinishNtuple();
      G4cout << "Created phase space ntuple for photon or proton" << G4endl;

      //Creat second ntuple for electron
      man->CreateNtuple("e-","e- OutputNTuple");
      man->CreateNtupleIColumn("particle"); //int
      man->CreateNtupleIColumn("eventID");
      man->CreateNtupleDColumn("energies"); //double
      //man->CreateNtupleDColumn("momentumTheta");
      //man->CreateNtupleDColumn("momentumPhi");
      man->CreateNtupleDColumn("existPosTheta");
      man->CreateNtupleDColumn("existPosPhi");
      man->CreateNtupleDColumn("existPosRho");
      man->CreateNtupleDColumn("PrimaryMomentumTheta");


  //   man->CreateNtupleIColumn("processes");
  //    man->CreateNtupleIColumn("material");
  //    man->CreateNtupleIColumn("origin");
  //    man->CreateNtupleDColumn("depth");
      man->FinishNtuple();
      G4cout << "Created phase space ntuple for electron" << G4endl;
      man->OpenFile(outputFileName);  // it won't work if I do not open it again.


    }
  else {
    // Book histograms
    //500 bins, minimum 0, maximum 10 keV.
    man->CreateH1("h1","Energy Deposit", 500,0.,200.); //
  //  man->CreateH1("h2","Gamma born in the sample", 100,0.,200.);
  //  man->CreateH1("h3","Electrons  born in the sample", 100,0.,200.);
    man->CreateH1("h2","Gammas leaving the sample", 300,0.,200.);//it's so stupid!
    //I actually cannot specify ID!
    man->CreateH1("h3","Electrons leaving the sample ",200000 ,0.,200); //
    man->CreateH1("h4","Gammas reaching the detector", 100,0.,200);
   man->CreateH1("h5","Spectrum of the incident particles", 100,0.,2000);
  //  man->CreateH1("h8","Protons reaching the detector", 100,0.,200);
  //  man->CreateH1("h9","Protons leaving the sample", 100,0.,200);
    man->CreateH1("h6","Gammas reaching the collimator", 100,0.,100.);
    man->CreateH1("h7","Gammas reaching the Window", 100,0.,100.);
    G4cout << "Created histos" << G4endl;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
//Not useful until call /gun/loadGunData, see Primary
void XrayFluoAnalysisManager::LoadGunData(G4String fileName, G4bool raileighFlag)
{
  G4AutoLock l(&dataManipulationMutex);

  //Do not allow more than one thread to trigger the file reading
  if (dataLoaded)
    return;

  // Get analysis reader manager
  G4AnalysisReader* analysisReader = G4AnalysisReader::Instance();
  analysisReader->SetVerboseLevel(4);

  //This is for testing purposes
  G4int ntupleId = analysisReader->GetNtuple("101",fileName);
  G4cout << "Got ntupleId: " << ntupleId << G4endl;

  gunParticleEnergies = new std::vector<G4double>;
  gunParticleTypes = new std::vector<G4String>;

  G4int particleID, processID;
  G4double energy;
  analysisReader->SetNtupleIColumn("processes", processID);
  analysisReader->SetNtupleDColumn("energies", energy);
  analysisReader->SetNtupleIColumn("particles", particleID);

  while (analysisReader->GetNtupleRow() )
    {
      if (raileighFlag ^ (!raileighFlag && (processID == 1 ||
					    processID == 2)) )
	{
	  gunParticleEnergies->push_back(energy*MeV);
	  if (particleID == 1)
	    gunParticleTypes->push_back("gamma");
	  else if (particleID == 0)
	    gunParticleTypes->push_back("e-");
	}
    }

  G4cout << "Maximum mumber of events: "<< gunParticleEnergies->size() <<G4endl;

  dataLoaded= true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

const std::pair<G4double,G4String> XrayFluoAnalysisManager::GetEmittedParticleEnergyAndType()
{
  G4AutoLock l(&dataManipulationMutex);
  std::pair<G4double,G4String> result;

  if(fParticleEnergyAndTypeIndex < (G4int) gunParticleEnergies->size())
    {
      G4double energy = gunParticleEnergies->at(fParticleEnergyAndTypeIndex);
      G4String name = gunParticleTypes->at(fParticleEnergyAndTypeIndex);
      result.first = energy;
      result.second = name;
    }

  fParticleEnergyAndTypeIndex++;
  return result;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....


void XrayFluoAnalysisManager::finish()
{
  G4AutoLock l(&dataManipulationMutex);
  G4cout << "Going to save histograms" << G4endl;
  // Save histograms
  G4AnalysisManager* man = G4AnalysisManager::Instance();
  man->Write();
  man->CloseFile();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....


void XrayFluoAnalysisManager::SetPhysicFlag(G4bool val)
{
  physicFlag = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void XrayFluoAnalysisManager::analyseStepping(const G4Step* aStep)
{
  G4AutoLock l(&dataManipulationMutex);
  G4AnalysisManager* man = G4AnalysisManager::Instance();
  phaseSpaceFlag=1;
  if (phaseSpaceFlag){
    G4ParticleDefinition* particleType= 0;
    G4String parentProcess="";
    G4ThreeVector momentum(0.,0.,0.);
    G4ThreeVector existPos(0,0,0);
    G4double particleEnergy=0;
    G4String sampleMaterial="";
    G4double particleDepth=0;
    G4int isBornInTheSample=0;
    G4int evtNb = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
    G4ThreeVector primarymomentum(0,0,0);
   // XrayFluoDetectorConstruction* detector = XrayFluoDetectorConstruction::GetInstance();
    //if(aStep->GetPostStepPoint()->GetStepStatus() == fGeomBoundary)
    if(aStep->GetPostStepPoint()->GetStepStatus() == fWorldBoundary)
    {
      particleType = aStep->GetTrack()->GetDynamicParticle()->GetDefinition();
      momentum = aStep->GetTrack()->GetDynamicParticle()->GetMomentum();
      particleEnergy = aStep->GetPreStepPoint()->GetKineticEnergy();
      existPos=aStep->GetPostStepPoint()->GetPosition();

      primarymomentum = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetPrimaryVertex()->GetPrimary()->GetMomentumDirection();

      G4int part = -1 ;
      if (particleType == G4Gamma::Definition()) part =1;
      if (particleType == G4Electron::Definition()) part = 0;
      if (particleType == G4Proton::Definition()) part = 2;
      if (part!=0 )
      {
            man->FillNtupleIColumn(1,0, part);
            man->FillNtupleIColumn(1,1,evtNb);
        	  man->FillNtupleDColumn(1,2,particleEnergy);
        	 // man->FillNtupleDColumn(1,3,momentum.theta());
        	 // man->FillNtupleDColumn(1,4,momentum.phi());
        	 // man->FillNtupleIColumn(1,4,parent);
        	 // man->FillNtupleIColumn(1,5,sampleMat);
        	 // man->FillNtupleIColumn(1,6,isBornInTheSample);
        	 // man->FillNtupleDColumn(1,7,particleDepth);
            man->FillNtupleDColumn(1,3,existPos.theta());
            man->FillNtupleDColumn(1,4,existPos.phi());
            man->FillNtupleDColumn(1,5,existPos.rho()/m);
            man->FillNtupleDColumn(1,6,primarymomentum.theta());
        	  man->AddNtupleRow(1);
            photonnumber++;
            if (photonnumber==NtupleDataVolume)
            {      G4ExceptionDescription execp;
                  execp <<  "Collected enough photons.";
                  G4Exception("XrayFluoAnalysisManger","example-xray_fluorescence04",
            	  FatalException, execp);
            }
      } else { //part ==0 , electron
        if (electronnumber<NtupleDataVolume)
        {
            man->FillNtupleIColumn(2,0, part);
            man->FillNtupleIColumn(2,1,evtNb);
            man->FillNtupleDColumn(2,2,particleEnergy);
          //  man->FillNtupleDColumn(2,3,momentum.theta());
          //  man->FillNtupleDColumn(2,4,momentum.phi());
           // man->FillNtupleIColumn(1,4,parent);
           // man->FillNtupleIColumn(1,5,sampleMat);
           // man->FillNtupleIColumn(1,6,isBornInTheSample);
           // man->FillNtupleDColumn(1,7,particleDepth);
            man->FillNtupleDColumn(2,3,existPos.theta());
            man->FillNtupleDColumn(2,4,existPos.phi());
            man->FillNtupleDColumn(2,5,existPos.rho()/m);
            man->FillNtupleDColumn(1,6,primarymomentum.theta());
            man->AddNtupleRow(2);
            electronnumber++;
        }
        if (electronnumber==NtupleDataVolume)  man->FillNtupleIColumn(2,0, electronnumber);
      }
    }

    /*
    // Select volume from which the step starts
    if(aStep->GetPreStepPoint()->GetPhysicalVolume()->GetName()=="Sample"){
      G4ThreeVector creationPos = aStep->GetTrack()->GetVertexPosition();
      // qua serve ancora una selezione: deve essere interno al sample
      //codice "a mano" per controllare il volume di orgine della traccia

      G4VPhysicalVolume* creationPosVolume = detector->GetGeometryNavigator()->LocateGlobalPointAndSetup(creationPos);

      // if physicflag is on, I record all the data of all the particle born in the sample.
      // If it is off (but phase space production is on) I record data
      // only for particles creted inside the sample and exiting it.
      if (physicFlag ^ (!physicFlag &&
			(aStep->GetPostStepPoint()->GetStepStatus() == fGeomBoundary)
			))
	    {
	  // extracting information needed
	  particleType = aStep->GetTrack()->GetDynamicParticle()->GetDefinition();
	  momentum = aStep->GetTrack()->GetDynamicParticle()->GetMomentum();
	  particleEnergy = aStep->GetPreStepPoint()->GetKineticEnergy();
	  if (creationPosVolume->GetName() == "Sample" ) {
	    isBornInTheSample = 1;
	    particleDepth = creationPosVolume->GetLogicalVolume()->GetSolid()
	      ->DistanceToOut(creationPos, G4ThreeVector(0,0,-1));
	  }
	  else {
	    particleDepth = -1;
	  }
	  // metodo per ottenere la profondita' "a mano"
	  //	particleDepth = std::abs(creationPos.z() - detector->GetSampleIlluminatedFaceCoord());

	  G4int parent=0;
	  if(aStep->GetTrack()->GetCreatorProcess())
	    {
	      parentProcess = aStep->GetTrack()->GetCreatorProcess()->GetProcessName();
	      parent = 5;
	      // hack for HBK
	      if (parentProcess == "") parent = 0;
	      if (parentProcess == "ioni") parent = 1;
	      if (parentProcess == "LowEnPhotoElec") parent = 2;
	      if (parentProcess == "Transportation") parent = 3;// should never happen
	      if (parentProcess == "initStep") parent = 4;// should never happen
	    }
	  else {
	    parentProcess = "Not Known -- (primary generator + Rayleigh)";
	    parent = 5;
	  }
	  G4int sampleMat=0;
	  if(aStep->GetTrack()){
	    sampleMaterial = aStep->GetTrack()->GetMaterial()->GetName();
	    if (sampleMaterial == ("Dolorite" || "Anorthosite" || "Mars1" || "IceBasalt" || "HPGe")) sampleMat=1;
	  }

	  G4int part = -1 ;
	  if (particleType == G4Gamma::Definition()) part =1;
	  if (particleType == G4Electron::Definition()) part = 0;
	  if (particleType == G4Proton::Definition()) part = 2;

	  //Fill ntuple
	  man->FillNtupleIColumn(1,0, part);
	  man->FillNtupleDColumn(1,1,particleEnergy);
	  man->FillNtupleDColumn(1,2,momentum.theta());
	  man->FillNtupleDColumn(1,3,momentum.phi());
	  man->FillNtupleIColumn(1,4,parent);
	  man->FillNtupleIColumn(1,5,sampleMat);
	  man->FillNtupleIColumn(1,6,isBornInTheSample);
	  man->FillNtupleDColumn(1,7,particleDepth);
	  man->AddNtupleRow(1);

	}
    }
  }
*/
  // Normal behaviour, without creation of phase space
  }
  else //if not phaseSpace
    {

      // Filling the histograms with data, passing thru stepping.

      // Select volume from wich the step starts
      if(aStep->GetPreStepPoint()->GetPhysicalVolume()->GetName()=="Sample"){
	// Select volume from wich the step ends
	if(aStep->GetTrack()->GetNextVolume()->GetName() == "World" ) {
	  // Select the particle type
	  if ((aStep->GetTrack()->GetDynamicParticle()
	       ->GetDefinition()) == G4Gamma::Definition() )

	    {
	      G4double gammaLeavingSample =
		aStep->GetPreStepPoint()->GetKineticEnergy();
	      man->FillH1(2,gammaLeavingSample/keV);

	    }
	  else if ((aStep->GetTrack()->GetDynamicParticle()
		    ->GetDefinition()) == G4Electron::Definition() )
	    {
	      G4double eleLeavingSample =
		aStep->GetPreStepPoint()->GetKineticEnergy();
	      man->FillH1(3,eleLeavingSample/keV);
	    }
      /*
	  else if ((aStep->GetTrack()->GetDynamicParticle()
		    ->GetDefinition()) == G4Proton::Definition() )
	    {
	      G4double
		protonsLeavSam = aStep->GetPreStepPoint()->GetKineticEnergy();
	      man->FillH1(9,protonsLeavSam/keV);
	    }
      */
	}
      }
/*
      if((aStep->GetTrack()->GetDynamicParticle()
	  ->GetDefinition()) == G4Gamma::Definition() )
        {

	  if(aStep->GetTrack()->GetCurrentStepNumber() == 1)
	    {
	      if(aStep->GetTrack()->GetParentID() != 0)
		{

		  if(aStep->GetTrack()->GetVolume()->GetName() == "Sample")
		    {
		      G4double gammaBornInSample =
			aStep->GetPreStepPoint()->GetKineticEnergy();
		      man->FillH1(2,gammaBornInSample/keV);
		    }
		}
	    }

	}
      else if ((aStep->GetTrack()->GetDynamicParticle()
	    ->GetDefinition() ) == G4Electron::Definition() )
	{
	  if(aStep->GetTrack()->GetCurrentStepNumber() == 1)
	    {
	      if(aStep->GetTrack()->GetParentID() != 0)
		{
		  if(aStep->GetTrack()->GetVolume()->GetName() == "Sample")
		    {
		      G4double eleBornInSample =
			aStep->GetPreStepPoint()->GetKineticEnergy();
		      man->FillH1(3,eleBornInSample/keV);
		    }
		}
	    }
	}
*/

      if(aStep->GetTrack()->GetNextVolume())
	{

    if(aStep->GetTrack()->GetNextVolume()->GetName() == "Collimator")

     {
           if ((aStep->GetTrack()->GetDynamicParticle()
          ->GetDefinition()) == G4Gamma::Definition() )
           {
             G4double gammaAtTheDetPre =
               aStep->GetPreStepPoint()->GetKineticEnergy();
             man->FillH1(6,gammaAtTheDetPre/keV);
           }
     }
     else if(aStep->GetTrack()->GetNextVolume()->GetName() == "Window")
      {
            if ((aStep->GetTrack()->GetDynamicParticle()
           ->GetDefinition()) == G4Gamma::Definition() )
            {
              G4double gammaAtTheDetPre =
                aStep->GetPreStepPoint()->GetKineticEnergy();
              man->FillH1(7,gammaAtTheDetPre/keV);
            }
      }
	  else if(aStep->GetTrack()->GetNextVolume()->GetName() == "TeflonBlock")

	    {
	      if ((aStep->GetTrack()->GetDynamicParticle()
		   ->GetDefinition()) == G4Gamma::Definition() )
		{

		  G4double gammaAtTheDetPre =
		    aStep->GetPreStepPoint()->GetKineticEnergy();
		  man->FillH1(4,gammaAtTheDetPre/keV);
		}
    /*
	      else if ((aStep->GetTrack()->GetDynamicParticle()
			->GetDefinition() ) == G4Proton::Definition() )
		{
		  G4double protonsAtTheDetPre =
		    aStep->GetPreStepPoint()->GetKineticEnergy();
		  man->FillH1(8,protonsAtTheDetPre);
		}
    */
	    }

	}
    }

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void XrayFluoAnalysisManager::analyseEnergyDep(G4double energyDep)
{
  G4AutoLock l(&dataManipulationMutex);
  // Filling of Energy Deposition, called by XrayFluoEventAction
  G4AnalysisManager* man = G4AnalysisManager::Instance();

  if (!phaseSpaceFlag)
    man->FillH1(1,energyDep/keV);

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void XrayFluoAnalysisManager::analysePrimaryGenerator(G4double energy)
{
  G4AutoLock l(&dataManipulationMutex);
  // Filling of energy spectrum histogram of the primary generator
  G4AnalysisManager* man = G4AnalysisManager::Instance();
  if (!phaseSpaceFlag)
    man->FillH1(5,energy/keV);
//  G4cout<<energy/keV<<" "<<energy<<" "<<keV<<G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void XrayFluoAnalysisManager::SetOutputFileName(G4String newName)
{

  outputFileName = newName;
}

void XrayFluoAnalysisManager::SetDataVolume(G4int DataVolume)
{
  NtupleDataVolume=DataVolume;
}
