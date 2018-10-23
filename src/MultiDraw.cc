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

#include <stdexcept>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

multidraw::MultiDraw::MultiDraw(char const* _treeName/* = "events"*/) :
  tree_(_treeName),
  library_(tree_)
{
  cuts_.push_back(new Cut("", [](unsigned)->double { return 1.; }));
}

multidraw::MultiDraw::~MultiDraw()
{
  tree_.SetEntryList(nullptr);

  for (auto* cut : cuts_)
    delete cut;

  for (auto* chain : friendTrees_) {
    tree_.RemoveFriend(chain);
    delete chain;
  }
}

void
multidraw::MultiDraw::addFriend(char const* _treeName, TObjArray const* _paths)
{
  auto* chain(new TChain(_treeName));
  for (auto* path : *_paths)
    chain->Add(path->GetName());

  long nBase(tree_.GetEntries());
  long nFriend(chain->GetEntries());

  if (nFriend != nBase) {
    std::stringstream ss;
    ss << "Friend tree entry number mismatch:";
    ss << " base tree " << nBase << " != " << " friend tree " << nFriend;
    std::cout << ss.str() << std::endl;
    throw std::runtime_error(ss.str());
  }

  friendTrees_.push_back(chain);

  tree_.AddFriend(chain);
}

void
multidraw::MultiDraw::setFilter(char const* _expr)
{
  findCut_("").setFormula(library_.getFormula(_expr));
}

void
multidraw::MultiDraw::addCut(char const* _name, char const* _expr)
{
  auto cutItr(std::find_if(cuts_.begin(), cuts_.end(), [_name](Cut* const& _cut)->bool { return _cut->getName() == _name; }));

  if (cutItr != cuts_.end()) {
    std::stringstream ss;
    ss << "Cut named " << _name << " already exists";
    std::cout << ss.str() << std::endl;
    throw std::invalid_argument(ss.str());
  }

  cuts_.push_back(new Cut(_name, library_.getFormula(_expr)));
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
multidraw::MultiDraw::setConstantWeight(double _w, int _treeNumber/* = -1*/, bool _exclusive/* = true*/)
{
  if (_treeNumber >= 0)
    treeWeight_[_treeNumber] = std::make_pair(_w, _exclusive);
  else
    globalWeight_ = _w;
}

void
multidraw::MultiDraw::setReweight(char const* _expr, TObject const* _source/* = nullptr*/, int _treeNumber/* = -1*/, bool _exclusive/* = true*/)
{
  Evaluable& reweight(_treeNumber >= 0 ? treeReweight_[_treeNumber].first : globalReweight_);

  reweight.reset();

  if (_treeNumber >= 0)
    treeReweight_[_treeNumber].second = _exclusive;

  if (!_expr || std::strlen(_expr) == 0)
    return;

  auto& formula(library_.getFormula(_expr));

  if (_source == nullptr) {
    // Expression is directly the reweight factor
    reweight.set(formula);
  }
  else {
    // Otherwise we evaluate the value of the formula over the source

    Evaluable::InstanceVal instanceVal;

    if (_source->InheritsFrom(TH1::Class())) {
      auto* source(static_cast<TH1 const*>(_source));

      instanceVal = [&formula, source](unsigned iD)->double {
        double x(formula->EvalInstance(iD));

        int iX(source->FindFixBin(x));
        if (iX == 0)
          iX = 1;
        else if (iX > source->GetNbinsX())
          iX = source->GetNbinsX();

        return source->GetBinContent(iX);
      };
    }
    else if (_source->InheritsFrom(TGraph::Class())) {
      auto* source(static_cast<TGraph const*>(_source));

      int n(source->GetN());
      double* xvals(source->GetX());
      for (int i(0); i != n - 1; ++i) {
        if (xvals[i] >= xvals[i + 1])
          throw std::runtime_error("Reweight TGraph source must have xvalues in increasing order");
      }

      instanceVal = [&formula, source](unsigned iD)->double {
        double x(formula->EvalInstance(iD));

        int n(source->GetN());
        double* xvals(source->GetX());

        double* b(std::upper_bound(xvals, xvals + n, x));
        if (b == xvals + n)
          return source->GetY()[n - 1];
        else if (b == xvals)
          return source->GetY()[0];
        else {
          // interpolate
          int low(b - xvals - 1);
          double dlow(x - xvals[low]);
          double dhigh(xvals[low + 1] - x);

          return (source->GetY()[low] * dhigh + source->GetY()[low + 1] * dlow) / (xvals[low + 1] - xvals[low]);
        }
      };
    }
    else if (_source->InheritsFrom(TF1::Class())) {
      auto* source(static_cast<TF1 const*>(_source));

      instanceVal = [&formula, source](unsigned iD)->double {
        return source->Eval(formula->EvalInstance(iD));
      };
    }
    else
      throw std::runtime_error("Incompatible object passed as reweight source");

    Evaluable::NData ndata{nullptr};
    if (formula->GetMultiplicity() != 0)
      ndata = [&formula]()->unsigned { return formula->GetNdata(); };

    reweight.set(instanceVal, ndata);
  }
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
  auto& exprFormula(library_.getFormula(_expr));

  auto newPlot1D([_hist, _overflowMode, &exprFormula](TTreeFormulaCachedPtr const& _reweightFormula)->ExprFiller* {
      return new Plot1DFiller(*_hist, exprFormula, _reweightFormula, _overflowMode);
    });

  if (printLevel_ > 1) {
    std::cout << "\nAdding Plot " << _hist->GetName() << " with expression " << _expr << std::endl;
    if (_cutName != nullptr && std::strlen(_cutName) != 0)
      std::cout << " Cut: " << _cutName << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  return static_cast<Plot1DFiller&>(addObj_(_cutName, _reweight, newPlot1D));
}

multidraw::Plot2DFiller&
multidraw::MultiDraw::addPlot2D(TH2* _hist, char const* _xexpr, char const* _yexpr, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/)
{
  auto& xexprFormula(library_.getFormula(_xexpr));
  auto& yexprFormula(library_.getFormula(_yexpr));

  auto newPlot2D([_hist, &xexprFormula, &yexprFormula](TTreeFormulaCachedPtr const& _reweightFormula)->ExprFiller* {
      return new Plot2DFiller(*_hist, xexprFormula, yexprFormula, _reweightFormula);
    });

  if (printLevel_ > 1) {
    std::cout << "\nAdding Plot " << _hist->GetName() << " with expression " << _yexpr << ":" << _xexpr << std::endl;
    if (_cutName != nullptr && std::strlen(_cutName) != 0)
      std::cout << " Cut: " << _cutName << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  return static_cast<Plot2DFiller&>(addObj_(_cutName, _reweight, newPlot2D));
}

multidraw::TreeFiller&
multidraw::MultiDraw::addTree(TTree* _tree, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/)
{
  auto newTree([this, _tree](TTreeFormulaCachedPtr const& _reweightFormula)->ExprFiller* {
      return new TreeFiller(*_tree, this->library_, _reweightFormula);
    });

  if (printLevel_ > 1) {
    std::cout << "\nAdding Tree " << _tree->GetName() << std::endl;
    if (_cutName != nullptr && std::strlen(_cutName) != 0)
      std::cout << " Cut: " << _cutName << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  return static_cast<TreeFiller&>(addObj_(_cutName, _reweight, newTree));
}

multidraw::ExprFiller&
multidraw::MultiDraw::addObj_(TString const& _cutName, char const* _reweight, ObjGen const& _gen)
{
  auto& cut(findCut_(_cutName));

  TTreeFormulaCachedPtr reweightFormula(nullptr);
  if (_reweight != nullptr && std::strlen(_reweight) != 0)
    reweightFormula = library_.getFormula(_reweight);

  auto* filler(_gen(reweightFormula));

  cut.addFiller(*filler);

  return *filler;
}

multidraw::Cut&
multidraw::MultiDraw::findCut_(TString const& _cutName) const
{
  auto cutItr(std::find_if(cuts_.begin(), cuts_.end(), [&_cutName](Cut* const& _cut)->bool { return _cut->getName() == _cutName; }));

  if (cutItr == cuts_.end()) {
    std::stringstream ss;
    ss << "Cut \"" << _cutName << "\" not defined";
    std::cerr << ss.str() << std::endl;
    throw std::runtime_error(ss.str());
  }

  return **cutItr;
}

void
multidraw::MultiDraw::execute(long _nEntries/* = -1*/, long _firstEntry/* = 0*/)
{
  long printEvery(10000);
  if (printLevel_ == 2)
    printEvery = 10000;
  else if (printLevel_ == 3)
    printEvery = 100;
  else if (printLevel_ >= 4)
    printEvery = 1;

  // Select only the cuts with at least one filler
  std::vector<Cut*> cuts;

  for (auto* cut : cuts_) {
    if (cut->getName().Length() != 0 && cut->getNFillers() == 0)
      continue;
    
    cut->setPrintLevel(printLevel_);
    cut->resetCount();

    cuts.push_back(cut);
  }

  // Also delete unused formulas
  // Cannot do this because there are formulas captured by lambdas
  //  library_.prune();

  std::vector<double> eventWeights;

  long long iEntry(_firstEntry);
  long long iEntryMax(_firstEntry + _nEntries);
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
  Evaluable* treeReweight{nullptr};
  bool exclusiveTreeReweight(false);
  
  while (iEntry != iEntryMax) {
    // iEntryNumber != iEntry if tree has a TEntryList set
    long long iEntryNumber(tree_.GetEntryNumber(iEntry++));
    if (iEntryNumber < 0)
      break;

    long long iLocalEntry(tree_.LoadTree(iEntryNumber));
    if (iLocalEntry < 0)
      break;

    if (printLevel_ >= 0 && iEntry % printEvery == 0) {
      std::cout << "\r      " << iEntry << " events";
      if (printLevel_ > 2)
        std::cout << std::endl;
    }

    if (treeNumber != tree_.GetTreeNumber()) {
      if (printLevel_ > 1)
        std::cout << "      Opened a new file: " << tree_.GetCurrentFile()->GetName() << std::endl;

      treeNumber = tree_.GetTreeNumber();

      if (weightBranchName_.Length() != 0) {
        weightBranch = tree_.GetBranch(weightBranchName_);
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
        evtNumBranch = tree_.GetBranch(evtNumBranchName_);
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

      for (auto& ff : library_)
        ff.second->UpdateFormulaLeaves();

      auto wItr(treeWeight_.find(treeNumber));
      if (wItr == treeWeight_.end())
        treeWeight = globalWeight_;
      else if (wItr->second.second) // exclusive tree-by-tree weight
        treeWeight = wItr->second.first;
      else
        treeWeight = globalWeight_ * wItr->second.first;

      auto rItr(treeReweight_.find(treeNumber));
      if (rItr == treeReweight_.end()) {
        if (globalReweight_.isValid())
          treeReweight = &globalReweight_;
        else
          treeReweight = nullptr;

        exclusiveTreeReweight = true;
      }
      else {
        treeReweight = &rItr->second.first;
        exclusiveTreeReweight = rItr->second.second;
      }
    }

    if (prescale_ > 1) {
      if (evtNumBranch != nullptr)
        evtNumBranch->GetEntry(iLocalEntry);

      if (printLevel_ > 3)
        std::cout << "        Event number " << getEvtNum() << std::endl;

      if (getEvtNum() % prescale_ != 0)
        continue;
    }

    // Reset formula cache
    for (auto& ff : library_)
      ff.second.get()->ResetCache();

    // First cut (name "") is a global filter
    if (!cuts[0]->evaluate())
      continue;

    if (weightBranch != nullptr) {
      weightBranch->GetEntry(iLocalEntry);

      if (printLevel_ > 3)
        std::cout << "        Input weight " << getWeight() << std::endl;
    }

    double commonWeight(getWeight() * treeWeight);

    if (treeReweight != nullptr) {
      unsigned nD(treeReweight->getNdata());
      if (nD == 0)
        continue; // skip event

      if (!exclusiveTreeReweight && globalReweight_.isValid()) {
        // just to load the values
        globalReweight_.getNdata();
      }

      eventWeights.resize(nD);

      for (unsigned iD(0); iD != nD; ++iD) {
        eventWeights[iD] = treeReweight->evalInstance(iD) * commonWeight;
        if (!exclusiveTreeReweight && globalReweight_.isValid())
          eventWeights[iD] *= globalReweight_.evalInstance(iD);
      }
    }
    else
      eventWeights.assign(1, commonWeight);

    if (printLevel_ > 3) {
      std::cout << "         Global weights: ";
      for (double w : eventWeights)
        std::cout << w << " ";
      std::cout << std::endl;
    }

    cuts[0]->fillExprs(eventWeights);
    for (unsigned iC(1); iC != cuts.size(); ++iC) {
      if (cuts[iC]->evaluate())
        cuts[iC]->fillExprs(eventWeights);
    }
  }

  totalEvents_ = iEntry;

  if (printLevel_ >= 0) {
    std::cout << "\r      " << iEntry << " events";
    std::cout << std::endl;
  }

  if (printLevel_ > 0) {
    for (auto* cut : cuts) {
      std::cout << "        Cut " << cut->getName() << ": passed total " << cut->getCount() << std::endl;
      for (unsigned iF(0); iF != cut->getNFillers(); ++iF) {
        auto* filler(cut->getFiller(iF));
        std::cout << "          " << filler->getObj().GetName() << ": " << filler->getCount() << std::endl;
      }
    }
  }
}
