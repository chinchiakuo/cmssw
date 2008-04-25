#include "RecoParticleFlow/PFClusterTools/interface/TreeUtility.hh"
#include "TBranch.h"
#include "TTree.h"
#include "RecoParticleFlow/PFClusterTools/interface/SingleParticleWrapper.hh"
using namespace minimiser;
TreeUtility::TreeUtility() {
}

TreeUtility::~TreeUtility() {
}

void TreeUtility::recreateFromRootFile(TFile& file,
		std::vector<DetectorElement* >& elements,
		std::vector<ParticleDeposit* >& toBeFilled) {

	std::cout << __PRETTY_FUNCTION__
			<< ": This method is highly specific to detector element types and may fail if their definitions change. Please be advised of this limitation!\n";

	DetectorElement* ecal(0);
	DetectorElement* hcal(0);
	DetectorElement* offset(0);
	SingleParticleWrapper* spw = new SingleParticleWrapper;

	for (std::vector<DetectorElement*>::iterator it = elements.begin(); it
			!= elements.end(); ++it) {
		DetectorElement* de = *it;
		if (de->getType() == ECAL)
			ecal = de;
		if (de->getType() == HCAL)
			hcal = de;
		if (de->getType() == OFFSET)
			offset = de;

	}
	if (offset == 0) {
		std::cout
				<< "Offset element NOT found in input collection; no 'a' coefficient will be added to ParticleDeposits.\n";
	}

	std::cout << "Initialised detector elements."<< std::endl;

	if (offset != 0)
		std::cout << "\t"<< *offset << "\n";
	std::cout << "\t"<< *ecal << "\n";
	std::cout << "\t"<< *hcal << "\n";
	file.ls();
	TTree* tree = (TTree*) file.Get("CaloData2");
	tree->ls();
	if (tree == 0) {
		MinimiserException me("Couldn't open tree!");
		throw me;
	}
	std::cout << "Opened Tree CaloData2...\n";
	

	std::cout << "Assigning branch: \n";
	TBranch* spwBr = tree->GetBranch("SingleParticleWrapper");
	spwBr->SetAddress(&spw);
	std::cout << "Looping over entries...\n";
	for (unsigned entries(0); entries < tree->GetEntries(); entries++) {

		tree->GetEntry(entries);

		ParticleDeposit* pd = new ParticleDeposit(spw->trueEnergy);
		if (offset != 0) {
			Deposition dOffset(offset, spw->eta, spw->phi, 1.0);
			pd->addRecDeposition(dOffset);
			pd->addTruthDeposition(dOffset);
		}
		Deposition dE(ecal, spw->eta, spw->phi, spw->eEcal);
		Deposition dH(hcal, spw->eta, spw->phi, spw->eHcal);

		//RecDepositions are what the detector element should detect
		//when well calibrated
		//yet includes detector effects

		pd->addRecDeposition(dE);
		pd->addRecDeposition(dH);

		//TruthDepositions are the true MC particle energy depositions
		//Here we set the same as rec depositions - they aren't used in this test case.

		pd->addTruthDeposition(dE);
		pd->addTruthDeposition(dH);

		std::cout << *pd << std::endl;
		toBeFilled.push_back(pd);

	}

}
void TreeUtility::recreateFromRootFile(TFile& f) {
	
	DetectorElement* ecal = new DetectorElement(ECAL, 1.0);
	DetectorElement* hcal = new DetectorElement(HCAL, 1.0);
	std::vector<DetectorElement*> elements;
	elements.push_back(ecal);
	elements.push_back(hcal);
	std::cout << "Made detector elements...\n";
	std::cout << "Recreating from root file...\n";
	std::vector<ParticleDeposit*> particles;
	recreateFromRootFile(f,elements,particles);
	std::cout << "Finished.\n";
}

