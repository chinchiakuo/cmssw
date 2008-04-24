#include <functional>
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <map>

#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/InputTag.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "SimDataFormats/JetMatching/interface/JetFlavourMatching.h"

#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/BTauReco/interface/JetTagInfo.h"
#include "DataFormats/BTauReco/interface/TaggingVariable.h"

#include "CondFormats/PhysicsToolsObjects/interface/MVAComputer.h"
#include "CondFormats/DataRecord/interface/BTauGenericMVAJetTagComputerRcd.h"

#include "PhysicsTools/MVATrainer/interface/MVATrainer.h"

#include "RecoBTau/JetTagComputer/interface/JetTagComputerRecord.h"
#include "RecoBTau/JetTagComputer/interface/GenericMVAComputer.h"
#include "RecoBTau/JetTagComputer/interface/GenericMVAComputerCache.h"
#include "RecoBTau/JetTagComputer/interface/TagInfoMVACategorySelector.h"
#include "RecoBTau/JetTagMVALearning/interface/JetTagMVATrainer.h"

using namespace reco;
using namespace PhysicsTools;

static const AtomicId kJetPt(TaggingVariableTokens[btau::jetPt]);
static const AtomicId kJetEta(TaggingVariableTokens[btau::jetEta]);

JetTagMVATrainer::JetTagMVATrainer(const edm::ParameterSet &params) :
	jetFlavour(params.getParameter<edm::InputTag>("jetFlavourMatching")),
	minPt(params.getParameter<double>("minimumTransverseMomentum")),
	minEta(params.getParameter<double>("minimumPseudoRapidity")),
	maxEta(params.getParameter<double>("maximumPseudoRapidity")),
	setupDone(false),
	jetTagComputer(params.getParameter<std::string>("jetTagComputer")),
	signalFlavours(params.getParameter<std::vector<int> >("signalFlavours")),
	ignoreFlavours(params.getParameter<std::vector<int> >("ignoreFlavours"))
{
	std::sort(signalFlavours.begin(), signalFlavours.end());
	std::sort(ignoreFlavours.begin(), ignoreFlavours.end());

	std::vector<std::string> calibrationLabels;
	if (params.getParameter<bool>("useCategories")) {
		categorySelector = std::auto_ptr<TagInfoMVACategorySelector>(
				new TagInfoMVACategorySelector(params));

		calibrationLabels = categorySelector->getCategoryLabels();
	} else {
		std::string calibrationRecord =
			params.getParameter<std::string>("calibrationRecord");

		calibrationLabels.push_back(calibrationRecord);
	}

	computerCache = std::auto_ptr<GenericMVAComputerCache>(
			new GenericMVAComputerCache(calibrationLabels));

	std::vector<std::string> inputTags =
			params.getParameterNamesForType<edm::InputTag>();

	for(std::vector<std::string>::const_iterator iter = inputTags.begin();
	    iter != inputTags.end(); iter++)
		tagInfoLabels[*iter] =
				params.getParameter<edm::InputTag>(*iter);
}

JetTagMVATrainer::~JetTagMVATrainer()
{
}

void JetTagMVATrainer::setup(const JetTagComputer &computer)
{
	std::vector<std::string> inputLabels(computer.getInputLabels());

	if (inputLabels.empty())
		inputLabels.push_back("tagInfo");

	for(std::vector<std::string>::const_iterator iter = inputLabels.begin();
	    iter != inputLabels.end(); iter++) {
		std::map<std::string, edm::InputTag>::const_iterator pos =
						tagInfoLabels.find(*iter);
		if (pos == tagInfoLabels.end())
			throw cms::Exception("InputTagMissing")
				<< "JetTagMVATrainer is missing a TagInfo "
				   "InputTag \"" << *iter << "\"" << std::endl;

		tagInfos.push_back(pos->second);
	}

	setupDone = true;
}

bool JetTagMVATrainer::isSignalFlavour(int flavour) const
{
	std::vector<int>::const_iterator pos =
		std::lower_bound(signalFlavours.begin(), signalFlavours.end(),
		                 flavour);

	return pos != signalFlavours.end() && *pos == flavour;
}

bool JetTagMVATrainer::isIgnoreFlavour(int flavour) const
{
	std::vector<int>::const_iterator pos =
		std::lower_bound(ignoreFlavours.begin(), ignoreFlavours.end(),
		                 flavour);

	return pos != ignoreFlavours.end() && *pos == flavour;
}

// map helper
namespace {
	struct JetCompare :
		public std::binary_function<edm::RefToBase<Jet>,
		                            edm::RefToBase<Jet>, bool> {
		inline bool operator () (const edm::RefToBase<Jet> &j1,
		                         const edm::RefToBase<Jet> &j2) const
		{ return j1.key() < j2.key(); }
	};

	struct JetInfo {
		unsigned int		flavour;
		std::vector<int>	tagInfos;
	};
}

void JetTagMVATrainer::analyze(const edm::Event& event,
                               const edm::EventSetup& es)
{
	// retrieve MVAComputer calibration container
	edm::ESHandle<Calibration::MVAComputerContainer> calibHandle;
	es.get<BTauGenericMVAJetTagComputerRcd>().get("trainer", calibHandle);
	const Calibration::MVAComputerContainer *calib = calibHandle.product();

	// check container for changes
	computerCache->update(calib);
	if (computerCache->isEmpty())
		return;

	// retrieve JetTagComputer
	edm::ESHandle<JetTagComputer> computerHandle;
	es.get<JetTagComputerRecord>().get(jetTagComputer, computerHandle);
	const GenericMVAJetTagComputer *computer =
			dynamic_cast<const GenericMVAJetTagComputer*>(
						computerHandle.product());
	if (!computer)
		throw cms::Exception("InvalidCast")
			<< "JetTagComputer is not a MVAJetTagComputer "
			   "in JetTagMVATrainer" << std::endl;

	computer->passEventSetup(es);

	// finalize the JetTagMVALearning <-> JetTagComputer glue setup
	if (!setupDone)
		setup(*computer);

	// retrieve TagInfos
	typedef std::map<edm::RefToBase<Jet>, JetInfo, JetCompare> JetInfoMap;
	JetInfoMap jetInfos;

	std::vector< edm::Handle< edm::View<BaseTagInfo> > >
					tagInfoHandles(tagInfos.size());
	unsigned int nTagInfos = tagInfos.size();
	for(unsigned int i = 0; i < nTagInfos; i++) {
		edm::Handle< edm::View<BaseTagInfo> > &tagInfoHandle =
							tagInfoHandles[i];
		event.getByLabel(tagInfos[i], tagInfoHandle);

		int j = 0;
		for(edm::View<BaseTagInfo>::const_iterator iter =
			tagInfoHandle->begin();
				iter != tagInfoHandle->end(); iter++, j++) {

			JetInfo &jetInfo = jetInfos[iter->jet()];
			if (jetInfo.tagInfos.empty()) {
				jetInfo.flavour = 0;
				jetInfo.tagInfos.resize(nTagInfos, -1);
			}

			jetInfo.tagInfos[i] = j;
		}
	}

	// retrieve jet flavours;
	edm::Handle<JetFlavourMatchingCollection> jetFlavourHandle;
	event.getByLabel(jetFlavour, jetFlavourHandle);

	for(JetFlavourMatchingCollection::const_iterator iter =
		jetFlavourHandle->begin();
				iter != jetFlavourHandle->end(); iter++) {

		JetInfoMap::iterator pos =
			jetInfos.find(edm::RefToBase<Jet>(iter->first));
		if (pos != jetInfos.end())
			pos->second.flavour = iter->second.getFlavour();
	}

	// cached array containing MVAComputer value list
	std::vector<Variable::Value> values;
	values.push_back(Variable::Value(MVATrainer::kTargetId, 0));
	values.push_back(Variable::Value(kJetPt, 0));
	values.push_back(Variable::Value(kJetEta, 0));

	// now loop over the map and compute all JetTags
	for(JetInfoMap::const_iterator iter = jetInfos.begin();
	    iter != jetInfos.end(); iter++) {
		edm::RefToBase<Jet> jet = iter->first;
		const JetInfo &info = iter->second;

		// simple jet filter
		if (jet->pt() < minPt ||
		    std::abs(jet->eta()) < minEta ||
		    std::abs(jet->eta()) > maxEta)
			continue;

		// do not train with unknown jet flavours
		if (isIgnoreFlavour(info.flavour))
			continue;

		// is it a b-jet?
		bool target = isSignalFlavour(info.flavour);

		// build TagInfos pointer for helper
		std::vector<const BaseTagInfo*> tagInfoPtrs(nTagInfos);
		for(unsigned int i = 0; i < nTagInfos; i++)  {
			if (info.tagInfos[i] < 0)
				continue;

			tagInfoPtrs[i] =
				&tagInfoHandles[i]->at(info.tagInfos[i]);
		}
		JetTagComputer::TagInfoHelper helper(tagInfoPtrs);

		TaggingVariableList variables =
					computer->taggingVariables(helper);

		// retrieve index of computer in case categories are used
		int index = 0;
		if (categorySelector.get()) {
			index = categorySelector->findCategory(variables);
			if (index < 0)
				continue;
		}

		GenericMVAComputer *mvaComputer =
					computerCache->getComputer(index);
		if (!mvaComputer)
			continue;

		// composite full array of MVAComputer values
		values.resize(3 + variables.size());
		std::vector<Variable::Value>::iterator insert = values.begin();

		(insert++)->setValue(target);
		(insert++)->setValue(jet->pt());
		(insert++)->setValue(jet->eta());
		std::copy(mvaComputer->iterator(variables.begin()),
		          mvaComputer->iterator(variables.end()), insert);

		static_cast<MVAComputer*>(mvaComputer)->eval(values);
	}
}
