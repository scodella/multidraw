#include "../interface/MultiDraw.h"

#include "TFile.h"
#include "TBranch.h"
#include "TGraph.h"
#include "TF1.h"
#include "TError.h"
#include "TLeafF.h"
#include "TLeafD.h"
#include "TLeafI.h"
#include "TLeafL.h"
#include "TEntryList.h"

#include <stdexcept>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <numeric>

multidraw::MultiDraw::MultiDraw(char const* _treeName/* = "events"*/) :
  treeName_(_treeName)
{
  cuts_.push_back(new Cut(""));
}

multidraw::MultiDraw::~MultiDraw()
{
  for (auto* cut : cuts_)
    delete cut;
}

void
multidraw::MultiDraw::addFriend(char const* _treeName, TObjArray const* _paths)
{
  auto* chain(new TChain(_treeName));
  for (auto* path : *_paths)
    chain->Add(path->GetName());

  friendTrees_.push_back(chain);
}

void
multidraw::MultiDraw::setFilter(char const* _expr)
{
  findCut_("").setCutExpr(_expr);
}

void
multidraw::MultiDraw::addCut(char const* _name, char const* _expr)
{
  if (_name == nullptr || std::strlen(_name) == 0)
    throw std::invalid_argument("Cannot add a cut with no name");

  auto cutItr(std::find_if(cuts_.begin(), cuts_.end(), [_name](Cut* const& _cut)->bool { return _cut->getName() == _name; }));

  if (cutItr != cuts_.end()) {
    std::stringstream ss;
    ss << "Cut named " << _name << " already exists";
    std::cout << ss.str() << std::endl;
    throw std::invalid_argument(ss.str());
  }

  cuts_.push_back(new Cut(_name, _expr));
}

void
multidraw::MultiDraw::addVariable(char const* _name, char const* _expr)
{
  if (_name == nullptr || std::strlen(_name) == 0)
    throw std::invalid_argument("Cannot add a variable with no name");

  variables_.emplace_back(_name, _expr);
}

void
multidraw::MultiDraw::removeCut(char const* _name)
{
  if (_name == nullptr || std::strlen(_name) == 0)
    throw std::invalid_argument("Cannot delete default cut");

  auto cutItr(std::find_if(cuts_.begin(), cuts_.end(), [_name](Cut* const& _cut)->bool { return _cut->getName() == _name; }));

  if (cutItr == cuts_.end()) {
    std::stringstream ss;
    ss << "Cut \"" << _name << "\" not defined";
    std::cerr << ss.str() << std::endl;
    throw std::runtime_error(ss.str());
  }

  cuts_.erase(cutItr);
  delete *cutItr;
}

void
multidraw::MultiDraw::setReweight(char const* _expr, TObject const* _source/* = nullptr*/)
{
  globalReweightExpr_ = _expr;
  globalReweightSource_ = _source;
}

void
multidraw::MultiDraw::setTreeReweight(int _treeNumber, bool _exclusive, char const* _expr, TObject const* _source/* = nullptr*/)
{
  treeReweightSources_[_treeNumber] = std::tuple<TString, TObject const*, bool>(_expr, _source, _exclusive);
}

void
multidraw::MultiDraw::setPrescale(unsigned _p, char const* _evtNumBranch/* = ""*/)
{
  if (_p == 0)
    throw std::invalid_argument("Prescale of 0 not allowed");
  prescale_ = _p;
  evtNumBranchName_ = _evtNumBranch;
}

multidraw::Plot1DFiller&
multidraw::MultiDraw::addPlot(TH1* _hist, char const* _expr, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/, Plot1DFiller::OverflowMode _overflowMode/* = kDefault*/)
{
  if (printLevel_ > 1) {
    std::cout << "\nAdding Plot " << _hist->GetName() << " with expression " << _expr << std::endl;
    if (_cutName != nullptr && std::strlen(_cutName) != 0)
      std::cout << " Cut: " << _cutName << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  auto& cut(findCut_(_cutName));

  auto* filler(new Plot1DFiller(*_hist, _expr, _reweight, _overflowMode));

  cut.addFiller(*filler);

  return *filler;
}

multidraw::Plot2DFiller&
multidraw::MultiDraw::addPlot2D(TH2* _hist, char const* _xexpr, char const* _yexpr, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/)
{
  if (printLevel_ > 1) {
    std::cout << "\nAdding Plot " << _hist->GetName() << " with expression " << _yexpr << ":" << _xexpr << std::endl;
    if (_cutName != nullptr && std::strlen(_cutName) != 0)
      std::cout << " Cut: " << _cutName << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  auto& cut(findCut_(_cutName));

  auto* filler(new Plot2DFiller(*_hist, _xexpr, _yexpr, _reweight));

  cut.addFiller(*filler);

  return *filler;
}

multidraw::TreeFiller&
multidraw::MultiDraw::addTree(TTree* _tree, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/)
{
  if (printLevel_ > 1) {
    std::cout << "\nAdding Tree " << _tree->GetName() << std::endl;
    if (_cutName != nullptr && std::strlen(_cutName) != 0)
      std::cout << " Cut: " << _cutName << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  auto& cut(findCut_(_cutName));

  auto* filler(new TreeFiller(*_tree, _reweight));

  cut.addFiller(*filler);

  return *filler;
}

multidraw::Cut&
multidraw::MultiDraw::findCut_(TString const& _cutName) const
{
  if (_cutName.Length() == 0)
    return *cuts_[0];

  auto cutItr(std::find_if(cuts_.begin(), cuts_.end(), [&_cutName](Cut* const& _cut)->bool { return _cut->getName() == _cutName; }));

  if (cutItr == cuts_.end()) {
    std::stringstream ss;
    ss << "Cut \"" << _cutName << "\" not defined";
    std::cerr << ss.str() << std::endl;
    throw std::runtime_error(ss.str());
  }

  return **cutItr;
}

unsigned
multidraw::MultiDraw::numObjs() const
{
  unsigned n(0);
  for (auto* cut : cuts_)
    n += cut->getNFillers();
  return n;
}

void
multidraw::MultiDraw::execute(long _nEntries/* = -1*/, unsigned long _firstEntry/* = 0*/)
{
  totalEvents_ = 0;

  int abortLevel(gErrorAbortLevel);
  if (doAbortOnReadError_)
    gErrorAbortLevel = kError;

  TChain mainTree(treeName_);

  for (auto& path : inputPaths_)
    mainTree.Add(path);

  mainTree.SetEntryList(entryList_);

  if (inputMultiplexing_ <= 1) {
    // Single-thread execution

    for (auto* frnd : friendTrees_)
      mainTree.AddFriend(frnd);

    totalEvents_ = executeOne_(_nEntries, _firstEntry, mainTree);
  }
  else {
    // Multi-thread execution

    if (!friendTrees_.empty()) {
      std::stringstream ss;
      ss << "MultiDraw does not know how to split the input by file when there are friend trees." << std::endl;
      std::cerr << ss.str() << std::endl;
      std::runtime_error(ss.str());
    }

    // Number of input files
    unsigned nTrees(mainTree.GetNtrees());

    // Actual file names (can be different from inputPaths_ which can include wildcards)
    std::vector<TString> fileNames;
    auto& fileElements(*mainTree.GetListOfFiles());
    for (auto* elem : fileElements)
      fileNames.emplace_back(elem->GetTitle());

    std::vector<TChain*> trees;
    std::vector<std::thread*> threads;
    SynchTools synchTools;
    synchTools.mainThread = std::this_thread::get_id();

    if (_nEntries == -1 && _firstEntry == 0 && nTrees > inputMultiplexing_) {
      // If there are more trees than threads and we are not limiting the number of entries to process,
      // we can split by file and avoid having to open all files up front with GetEntries.

      if (printLevel_ > 0) {
        std::cout << "Splitting task over " << nTrees;
        std::cout << " files in " << inputMultiplexing_ << " threads" << std::endl;
      }

      auto threadTask([this, &synchTools](unsigned _treeNumberOffset, TChain* _tree) {
#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
          this->executeOne_(-1, 0, *_tree, _treeNumberOffset, nullptr, &synchTools, true);
#else
          this->executeOne_(-1, 0, *_tree, _treeNumberOffset, &synchTools, true);
#endif
      });

      unsigned nFilesPerThread(nTrees / inputMultiplexing_);
      // main thread processes the residuals too
      unsigned nFilesMainThread(nTrees - nFilesPerThread * (inputMultiplexing_ - 1));

      for (unsigned iT(0); iT != inputMultiplexing_ - 1; ++iT) {
        auto* tree(new TChain(treeName_));

        unsigned treeNumberOffset(nFilesMainThread + iT * nFilesPerThread);
        TEntryList* threadElist{nullptr};

        if (entryList_ != nullptr) {
          threadElist = new TEntryList(tree);
          threadElist->SetDirectory(nullptr);
        }

        for (unsigned iS(treeNumberOffset); iS != treeNumberOffset + nFilesPerThread; ++iS) {
          auto& fileName(fileNames[iS]);
          tree->Add(fileName);
          if (threadElist != nullptr)
            threadElist->Add(entryList_->GetEntryList(treeName_, fileName));
        }

        if (threadElist != nullptr)
          tree->SetEntryList(threadElist);

        threads.push_back(new std::thread(threadTask, treeNumberOffset, tree));
        trees.push_back(tree);

        // Wait until the thread initialization step is done
        std::unique_lock<std::mutex> lock(synchTools.mutex);
        synchTools.condition.wait(lock, [&synchTools]() { return synchTools.flag.load(); });

        synchTools.flag = false;
      }

      // Started N-1 threads. Process the rest of events (staring from 0) in the main thread
#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
      executeOne_(nFilesMainThread, 0, mainTree, 0, nullptr, &synchTools, true);
#else
      executeOne_(nFilesMainThread, 0, mainTree, 0, &synchTools, true);
#endif
    }
    else{
      // If there are only a few files or if we are limiting the entries, we need to know how many
      // events are in each file

      if (printLevel_ > 0) {
        std::cout << "Fetching the total number of events from " << mainTree.GetNtrees();
        std::cout << " files to split the input " << inputMultiplexing_ << " ways" << std::endl;
      }

      // This step also fills the offsets array of the TChain
      unsigned long long nTotal(mainTree.GetEntries() - _firstEntry);

      if (entryList_ != nullptr)
        nTotal = entryList_->GetN() - _firstEntry;

      if (_nEntries >= 0 && _nEntries < (long long)(nTotal))
        nTotal = _nEntries;

      if (nTotal <= _firstEntry)
        return;

      long long nPerThread(nTotal / inputMultiplexing_);

      Long64_t* treeOffsets(mainTree.GetTreeOffset());

#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
      // treeOffsets are used in executeOne_ to identify tree transitions and lock the thread
      auto threadTask([this, nPerThread, treeOffsets, &synchTools](long _fE, unsigned _treeNumberOffset, TChain* _tree) {
          this->executeOne_(nPerThread, _fE, *_tree, _treeNumberOffset, treeOffsets, &synchTools);
      });
#else
      // Newer versions of ROOT is more thread-safe and does not require this lock
      auto threadTask([this, nPerThread, &synchTools](long _fE, unsigned _treeNumberOffset, TChain* _tree) {
          this->executeOne_(nPerThread, _fE, *_tree, _treeNumberOffset, &synchTools);
      });
#endif

      long firstEntry(_firstEntry); // first entry in the full chain
      for (unsigned iT(0); iT != inputMultiplexing_ - 1; ++iT) {
        auto* tree(new TChain(treeName_));

        unsigned treeNumberOffset(0);
        long threadFirstEntry(0);
        TEntryList* threadElist{nullptr};

        if (entryList_ == nullptr) {
          while (firstEntry >= treeOffsets[treeNumberOffset + 1])
            ++treeNumberOffset;

          threadFirstEntry = firstEntry - treeOffsets[treeNumberOffset];
        }
        else {
          long long n(0);
          for (auto* obj : *entryList_->GetLists()) {
            auto* el(static_cast<TEntryList*>(obj));
            if (firstEntry < n + el->GetN())
              break;
            ++treeNumberOffset;
            n += el->GetN();
          }

          threadFirstEntry = firstEntry - n;

          threadElist = new TEntryList(tree);
          threadElist->SetDirectory(nullptr);
        }

        // Add file names from treeNumberOffset to max, but may only use a part (depends on how many events the thread will process)
        for (unsigned iS(treeNumberOffset); iS != fileNames.size(); ++iS) {
          auto& fileName(fileNames[iS]);
          tree->Add(fileName);
          if (threadElist != nullptr)
            threadElist->Add(entryList_->GetEntryList(treeName_, fileName));
        }

        if (threadElist != nullptr)
          tree->SetEntryList(threadElist);

        std::unique_lock<std::mutex> lock(synchTools.mutex);

        threads.push_back(new std::thread(threadTask, threadFirstEntry, treeNumberOffset, tree));
        trees.push_back(tree);

        // Wait for initialization step to complete
        synchTools.condition.wait(lock, [&synchTools]() { return synchTools.flag.load(); });

        synchTools.flag = false;

        firstEntry += nPerThread;
      }

      // Started N-1 threads. Process the rest of events in the main thread
#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
      executeOne_(nTotal - (firstEntry - _firstEntry), firstEntry, mainTree, 0, treeOffsets, &synchTools);
#else
      executeOne_(nTotal - (firstEntry - _firstEntry), firstEntry, mainTree, 0, &synchTools);
#endif
    }

    {
      std::unique_lock<std::mutex> lock(synchTools.mutex);
      synchTools.flag = true;
      synchTools.condition.notify_all();
    }

    for (auto* thread : threads)
      thread->join();

    // Tree deletion should not be concurrent with THx deletion, which happens during the last part of executeOne_ (in Cut dtors)
    // Let all threads join first before destroying the trees
    for (unsigned iT(0); iT != inputMultiplexing_ - 1; ++iT) {
      delete threads[iT];
      auto* threadElist(trees[iT]->GetEntryList());
      trees[iT]->SetEntryList(nullptr);
      delete threadElist;
      delete trees[iT];
    }

    totalEvents_ = synchTools.totalEvents;
  }

  if (doAbortOnReadError_)
    gErrorAbortLevel = abortLevel;

  if (printLevel_ >= 0) {
    std::cout << "\r      " << totalEvents_ << " events" << std::endl;
    if (printLevel_ > 0) {
      for (unsigned iC(0); iC != cuts_.size(); ++iC) {
        auto& cut(*cuts_[iC]);
        if (iC != 0 && cut.getNFillers() == 0)
          continue;

        std::cout << "        Cut " << cut.getName() << ": passed total " << cut.getCount() << std::endl;
        if (printLevel_ > 1) {
          for (unsigned iF(0); iF != cut.getNFillers(); ++iF) {
            auto* filler(cut.getFiller(iF));
            std::cout << "          " << filler->getObj().GetName() << ": " << filler->getCount() << std::endl;
          }
        }
      }
    }
  }
}

typedef std::chrono::steady_clock SteadyClock;

double millisec(SteadyClock::duration const& interval)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count() * 1.e-6;
}

long
#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
multidraw::MultiDraw::executeOne_(long _nEntries, unsigned long _firstEntry, TChain& _tree, unsigned _treeNumberOffset/* = 0*/, Long64_t* _treeOffsets/* = nullptr*/, SynchTools* _synchTools/* = nullptr*/, bool _byTree/* = false*/)
#else
multidraw::MultiDraw::executeOne_(long _nEntries, unsigned long _firstEntry, TChain& _tree, unsigned _treeNumberOffset/* = 0*/, SynchTools* _synchTools/* = nullptr*/, bool _byTree/* = false*/)
#endif
{
  // treeNumberOffset: The offset of the given tree with respect to the original

  std::vector<SteadyClock::duration> cutTimers;
  SteadyClock::duration ioTimer(SteadyClock::duration::zero());
  SteadyClock::duration eventTimer(SteadyClock::duration::zero());
  SteadyClock::time_point start;

  int printLevel(-1);
  bool doTimeProfile(false);

  FormulaLibrary library(_tree);

  // If we have custom-defined variables, must compile them before cuts and fillers refer to them
  struct VariableSpec {
    TBranch* nbranch{nullptr};
    TBranch* vbranch{nullptr};
    unsigned nD{0};
    std::vector<double> values{};
    TTreeFormulaCachedPtr sourceFormula{};
  };

  TTree* variablesTree(nullptr);
  std::vector<VariableSpec> variables;

  auto compileVariables([&variablesTree, &variables, &library, &_tree, this]() {
      if (this->variables_.empty())
        return;

      TDirectory::TContext(nullptr);
      variablesTree = new TTree("_variables", "");
      variablesTree->SetCircular(10000); // buffer size 10000 to have fewer cycles

      _tree.AddFriend(variablesTree);

      variables.reserve(this->variables_.size());

      // Adding variables in given order - variables dependent on others must be declared in order
      for (auto& v : this->variables_) {
        auto& name(v.first);
        auto& expr(v.second);

        TTreeFormulaCachedPtr formula(library.getFormula(expr));

        variables.resize(variables.size() + 1);
        auto& varspec(variables.back());

        varspec.sourceFormula = formula;

        if (formula->GetMultiplicity() == 0) {
          // singlet branch
          varspec.values.resize(1);
          varspec.vbranch = variablesTree->Branch(name, varspec.values.data(), name + "/D");
        }
        else {
          // give some reasonable initial size
          varspec.values.resize(64);
          // array, or expression composed of dynamic array elements
          varspec.nbranch = variablesTree->Branch("size__" + name, &varspec.nD, "size__" + name + "/i");
          varspec.vbranch = variablesTree->Branch(name, varspec.values.data(), name + "[size__" + name + "]/D");
        }
      }
    });

  Reweight* globalReweight{nullptr};
  std::map<unsigned, std::pair<Reweight*, bool>> treeReweights;

  auto compileReweights([&globalReweight, &treeReweights, &library, this]() {
      if (this->globalReweightExpr_.Length() != 0) {
        if (this->globalReweightSource_ == nullptr)
          globalReweight = new Reweight(library.getFormula(this->globalReweightExpr_));
        else
          globalReweight = new Reweight(*this->globalReweightSource_, library.getFormula(this->globalReweightExpr_));
      }

      for (auto& tr : this->treeReweightSources_) {
        auto& formula(library.getFormula(std::get<0>(tr.second)));
        if (std::get<1>(tr.second) == nullptr)
          treeReweights[tr.first] = std::make_pair(new Reweight(formula), std::get<2>(tr.second));
        else
          treeReweights[tr.first] = std::make_pair(new Reweight(*std::get<1>(tr.second), formula), std::get<2>(tr.second));
      }
    });

  std::vector<Cut*> cuts;

  if (_synchTools == nullptr || std::this_thread::get_id() == _synchTools->mainThread) {
    // main thread
    printLevel = printLevel_;
    doTimeProfile = doTimeProfile_;

    compileVariables();

    for (unsigned iC(0); iC != cuts_.size(); ++iC) {
      auto& cut(*cuts_[iC]);
      if (iC != 0 && cut.getNFillers() == 0)
        continue;

      cut.setPrintLevel(printLevel);
      cut.bindTree(library);
    
      cuts.push_back(&cut);
    
      if (doTimeProfile)
        cutTimers.emplace_back(SteadyClock::duration::zero());
    }

    compileReweights();
  }
  else {
    // side threads

    // We need to perform all formula compilation under a lock because it uses a global
    std::lock_guard<std::mutex> lock(_synchTools->mutex);

    compileVariables();

    for (unsigned iC(0); iC != cuts_.size(); ++iC) {
      auto& origCut(*cuts_[iC]);
      if (iC != 0 && origCut.getNFillers() == 0)
        continue;

      cuts.push_back(origCut.threadClone(library));
    }

    compileReweights();

    // tell the main thread to go ahead with initializing the next thread
    _synchTools->flag = true;
    _synchTools->condition.notify_one();
  }

  std::vector<double> eventWeights;

  long long iEntry(0);
  int treeNumber(-1);

  union FloatingPoint {
    Float_t f;
    Double_t d;
  } weight;

  std::function<Double_t()> getWeight([]()->Double_t { return 1.; });

  union Integer {
    Int_t i;
    UInt_t ui;
    Long64_t l;
    ULong64_t ul;
  } evtNum;

  // by default, use iEntry as the event number
  std::function<ULong64_t()> getEvtNum([&iEntry]()->ULong64_t { return iEntry; });

  TBranch* weightBranch(nullptr);
  TBranch* evtNumBranch(nullptr);

  double treeWeight(1.);
  Reweight* treeReweight{nullptr};
  bool exclusiveTreeReweight(false);

#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
  Long64_t* treeOffsets(nullptr);

  if (_treeOffsets != nullptr) {
    if (_byTree) {
      // Jobs split by tree; tree offsets have not been calculated in the main thread
      _tree.GetEntries();
      treeOffsets = _tree.GetTreeOffset();
    }
    else {
      // nextTreeBoundary = _treeOffsets[_treeNumberOffset + treeNumber + 1] - _treeOffsets[_treeNumberOffset]
      //                  = treeOffsets[treeNumber + 1] - treeOffsets[0]
      treeOffsets = &(_treeOffsets[_treeNumberOffset]);
    }
  }

  long nextTreeBoundary(0);
#endif

  long nEntries(_byTree ? -1 : _nEntries);

  long printEvery(100000);
  if (printLevel_ == 3)
    printEvery = 1000;
  else if (printLevel_ >= 4)
    printEvery = 1;
  
  while (iEntry != nEntries) {
    if (iEntry % printEvery == 0) {
      if (_synchTools != nullptr)
        _synchTools->totalEvents += printEvery;

      if (printLevel >= 0) {
        if (_synchTools == nullptr)
          (std::cout << "\r      " << iEntry << " events").flush();
        else
          (std::cout << "\r      " << _synchTools->totalEvents.load() << " events").flush();

        if (printLevel > 2)
          std::cout << std::endl;
      }
    }

    if (doTimeProfile)
      start = SteadyClock::now();

    // iEntryNumber != iEntry if tree has a TEntryList set
    long long iEntryNumber(_tree.GetEntryNumber(iEntry + _firstEntry));
    if (iEntryNumber < 0)
      break;

#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
    long long iLocalEntry(0);
    
    if (treeOffsets != nullptr && iEntryNumber >= nextTreeBoundary) {
      // we are crossing a tree boundary in a multi-thread environment
      std::lock_guard<std::mutex> lock(_synchTools->mutex);
      iLocalEntry = _tree.LoadTree(iEntryNumber);
    }
    else {
      iLocalEntry = _tree.LoadTree(iEntryNumber);
    }
#else
    // newer ROOT versions can handle concurrent file transitions
    long long iLocalEntry(_tree.LoadTree(iEntryNumber));
#endif

    if (iLocalEntry < 0)
      break;

    ++iEntry;

    if (treeNumber != _tree.GetTreeNumber()) {
      if (_byTree && _nEntries >= 0 && _tree.GetTreeNumber() >= _nEntries) {
        // We are done
        break;
      }

      if (printLevel > 1)
        std::cout << "      Opened a new file: " << _tree.GetCurrentFile()->GetName() << std::endl;

      treeNumber = _tree.GetTreeNumber();

#if ROOT_VERSION_CODE < ROOT_VERSION(6,12,0)
      if (treeOffsets != nullptr)
        nextTreeBoundary = treeOffsets[treeNumber + 1] - treeOffsets[0];
#endif

      if (weightBranchName_.Length() != 0) {
        weightBranch = _tree.GetBranch(weightBranchName_);
        if (!weightBranch)
          throw std::runtime_error(("Could not find branch " + weightBranchName_).Data());

        auto* leaves(weightBranch->GetListOfLeaves());
        if (leaves->GetEntries() == 0) // shouldn't happen
          throw std::runtime_error(("Branch " + weightBranchName_ + " does not have any leaves").Data());

        weightBranch->SetAddress(&weight);

        auto* leaf(static_cast<TLeaf*>(leaves->At(0)));

        if (leaf->InheritsFrom(TLeafF::Class()))
          getWeight = [&weight]()->Double_t { return weight.f; };
        else if (leaf->InheritsFrom(TLeafD::Class()))
          getWeight = [&weight]()->Double_t { return weight.d; };
        else
          throw std::runtime_error(("I do not know how to read the leaf type of branch " + weightBranchName_).Data());
      }

      if (prescale_ > 1 && evtNumBranchName_.Length() != 0) {
        evtNumBranch = _tree.GetBranch(evtNumBranchName_);
        if (!evtNumBranch)
          throw std::runtime_error(("Could not find branch " + evtNumBranchName_).Data());

        auto* leaves(evtNumBranch->GetListOfLeaves());
        if (leaves->GetEntries() == 0) // shouldn't happen
          throw std::runtime_error(("Branch " + evtNumBranchName_ + " does not have any leaves").Data());

        evtNumBranch->SetAddress(&evtNum);

        auto* leaf(static_cast<TLeaf*>(leaves->At(0)));

        if (leaf->InheritsFrom(TLeafI::Class())) {
          if (leaf->IsUnsigned())
            getEvtNum = [&evtNum]()->ULong64_t { return evtNum.ui; };
          else
            getEvtNum = [&evtNum]()->ULong64_t { return evtNum.i; };
        }
        else if (leaf->InheritsFrom(TLeafL::Class())) {
          if (leaf->IsUnsigned())
            getEvtNum = [&evtNum]()->ULong64_t { return evtNum.ul; };
          else
            getEvtNum = [&evtNum]()->ULong64_t { return evtNum.l; };
        }
        else
          throw std::runtime_error(("I do not know how to read the leaf type of branch " + evtNumBranchName_).Data());
      }

      // Underlying tree changed; formulas must update their pointers
      for (auto& ff : library)
        ff.second->UpdateFormulaLeaves();

      // Constant overall tree weights
      auto wItr(treeWeights_.find(treeNumber + _treeNumberOffset));
      if (wItr == treeWeights_.end())
        treeWeight = globalWeight_;
      else if (wItr->second.second) // exclusive tree-by-tree weight
        treeWeight = wItr->second.first;
      else
        treeWeight = globalWeight_ * wItr->second.first;

      auto rItr(treeReweights.find(treeNumber + _treeNumberOffset));
      if (rItr == treeReweights.end()) {
        treeReweight = globalReweight;
        exclusiveTreeReweight = true;
      }
      else {
        treeReweight = rItr->second.first;
        exclusiveTreeReweight = (globalReweight == nullptr || rItr->second.second);
      }
    }

    if (prescale_ > 1) {
      if (evtNumBranch != nullptr)
        evtNumBranch->GetEntry(iLocalEntry);

      if (printLevel > 3)
        std::cout << "        Event number " << getEvtNum() << std::endl;

      if (getEvtNum() % prescale_ != 0)
        continue;
    }

    // Reset formula cache
    for (auto& ff : library)
      ff.second->ResetCache();

    if (doTimeProfile) {
      ioTimer += SteadyClock::now() - start;
      start = SteadyClock::now();
    }

    if (variablesTree != nullptr) {
      for (auto& v : variables) {
        if (v.sourceFormula->GetMultiplicity() == 0) {
          v.sourceFormula->GetNdata();
          v.values[0] = v.sourceFormula->EvalInstance(0);

          v.vbranch->Fill();
        }
        else {
          auto* currentData(v.values.data());
          v.nD = v.sourceFormula->GetNdata();
          v.values.resize(v.nD);

          if (v.values.data() != currentData) {
            // vector was reallocated
            v.vbranch->SetAddress(v.values.data());
          }

          for (unsigned iD(0); iD != v.nD; ++iD)
            v.values[iD] = v.sourceFormula->EvalInstance(iD);

          v.nbranch->Fill();
          v.vbranch->Fill();
        }
      }
    }

    // First cut (name "") is a global filter
    bool passFilter(cuts[0]->evaluate());

    if (doTimeProfile) {
      cutTimers[0] += SteadyClock::now() - start;
      start = SteadyClock::now();
    }

    if (!passFilter)
      continue;

    if (weightBranch != nullptr) {
      weightBranch->GetEntry(iLocalEntry);

      if (printLevel > 3)
        std::cout << "        Input weight " << getWeight() << std::endl;
    }

    if (doTimeProfile) {
      ioTimer += SteadyClock::now() - start;
      start = SteadyClock::now();
    }

    double commonWeight(getWeight() * treeWeight);

    if (treeReweight != nullptr) {
      unsigned nD(treeReweight->getNdata());
      if (nD == 0)
        continue; // skip event

      eventWeights.resize(nD);

      for (unsigned iD(0); iD != nD; ++iD) {
        eventWeights[iD] = treeReweight->evaluate(iD) * commonWeight;
        if (!exclusiveTreeReweight)
          eventWeights[iD] *= globalReweight->evaluate(iD);
      }
    }
    else {
      eventWeights.assign(1, commonWeight);
    }

    if (printLevel > 3) {
      std::cout << "         Global weights: ";
      for (double w : eventWeights)
        std::cout << w << " ";
      std::cout << std::endl;
    }

    if (doTimeProfile) {
      eventTimer += SteadyClock::now() - start;
      start = SteadyClock::now();
    }

    cuts[0]->fillExprs(eventWeights);

    if (doTimeProfile) {
      cutTimers[0] += SteadyClock::now() - start;
      start = SteadyClock::now();
    }

    for (unsigned iC(1); iC != cuts.size(); ++iC) {
      if (cuts[iC]->evaluate())
        cuts[iC]->fillExprs(eventWeights);

      if (doTimeProfile) {
        cutTimers[iC] += SteadyClock::now() - start;
        start = SteadyClock::now();
      }
    }
  }

  // Add the residual number of events
  if (_synchTools != nullptr)
    _synchTools->totalEvents += (iEntry % printEvery);

  if (printLevel >= 0 && doTimeProfile) {
    double totalTime(millisec(ioTimer) + millisec(eventTimer));
    totalTime += millisec(std::accumulate(cutTimers.begin(), cutTimers.end(), SteadyClock::duration::zero()));
    std::cout << " Execution time: " << (totalTime / iEntry) << " ms/evt" << std::endl;

    std::cout << "        Time spent on tree input: " << (millisec(ioTimer) / iEntry) << " ms/evt" << std::endl;
    std::cout << "        Time spent on event reweighting: " << (millisec(eventTimer) / iEntry) << " ms/evt" << std::endl;

    if (printLevel > 0) {
      for (unsigned iC(0); iC != cuts.size(); ++iC) {
        auto cut(*cuts[iC]);
        double cutTime(0.);
        if (iC == 0)
          cutTime = millisec(cutTimers[0]) / iEntry;
        else
          cutTime = millisec(cutTimers[iC]) / cuts[0]->getCount();
        std::cout << "        cut " << cut.getName() << ": " << cutTime << " ms/evt" << std::endl;
      }
    }
  }

  if (_synchTools == nullptr || std::this_thread::get_id() == _synchTools->mainThread) {
    // unlink

    for (auto* cut : cuts)
      cut->unlinkTree();
  }
  else {
    // merge & cleanup

    // Again we'll just lock the entire block
    std::unique_lock<std::mutex> lock(_synchTools->mutex);
    _synchTools->condition.wait(lock, [_synchTools]() { return _synchTools->flag.load(); });

    // Clone fillers will merge themselves to the main object in the destructor
    for (auto* cut : cuts)
      delete cut;
  }

  if (variablesTree != nullptr) {
    _tree.RemoveFriend(variablesTree);
    delete variablesTree;
  }

  delete globalReweight;
  for (auto& tr : treeReweights)
    delete tr.second.first;

  return iEntry;
}
