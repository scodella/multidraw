#include "../inc/MultiDraw.h"
#include "../inc/Plot1DFiller.h"
#include "../inc/TreeFiller.h"
#include "../inc/FormulaLibrary.h"

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

multidraw::MultiDraw::MultiDraw(char const* _treeName/* = "events"*/) :
  tree_(_treeName),
  library_(tree_)
{
  cuts_.push_back(new Cut("", [](UInt_t)->Double_t { return 1.; }));
}

multidraw::MultiDraw::~MultiDraw()
{
  for (auto* cut : cuts_)
    delete cut;
}

void
multidraw::MultiDraw::setFilter(char const* _expr)
{
  findCut_("").setFormula(*library_.getFormula(_expr));
}

void
multidraw::MultiDraw::addCut(char const* _name, char const* _expr)
{
  try {
    findCut_(_name);

    std::stringstream ss;
    ss << "Cut named " << _name << " already exists";
    std::cout << ss.str() << std::endl;
    throw std::invalid_argument(ss.str());
  }
  catch (std::runtime_error&) {
    // pass
  }

  cuts_.push_back(new Cut(_name, *library_.getFormula(_expr)));
}

void
multidraw::MultiDraw::setPrescale(unsigned _p, char const* _evtNumBranch/* = ""*/)
{
  if (_p == 0)
    throw std::invalid_argument("Prescale of 0 not allowed");
  prescale_ = _p;
  evtNumBranchName_ = _evtNumBranch;
}

void
multidraw::MultiDraw::setReweight(char const* _expr, TObject const* _source/* = nullptr*/)
{
  library_.deleteFormula(reweightExpr_);
  reweight_ = nullptr;

  if (!_expr || std::strlen(_expr) == 0)
    return;

  reweightExpr_ = library_.getFormula(_expr);

  if (_source != nullptr) {
    if (_source->InheritsFrom(TH1::Class())) {
      auto* source(static_cast<TH1 const*>(_source));

      reweight_ = [this, source](std::vector<double>& _values) {
        _values.clear();

        unsigned nD(this->reweightExpr_->GetNdata());

        for (unsigned iD(0); iD != nD; ++iD) {
          double x(this->reweightExpr_->EvalInstance(iD));

          int iX(source->FindFixBin(x));
          if (iX == 0)
            iX = 1;
          else if (iX > source->GetNbinsX())
            iX = source->GetNbinsX();

          _values.push_back(source->GetBinContent(iX));
        }
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

      reweight_ = [this, source](std::vector<double>& _values) {
        _values.clear();

        unsigned nD(this->reweightExpr_->GetNdata());

        for (unsigned iD(0); iD != nD; ++iD) {
          double x(this->reweightExpr_->EvalInstance(iD));

          int n(source->GetN());
          double* xvals(source->GetX());

          double* b(std::upper_bound(xvals, xvals + n, x));
          if (b == xvals + n)
            _values.push_back(source->GetY()[n - 1]);
          else if (b == xvals)
            _values.push_back(source->GetY()[0]);
          else {
            // interpolate
            int low(b - xvals - 1);
            double dlow(x - xvals[low]);
            double dhigh(xvals[low + 1] - x);

            _values.push_back((source->GetY()[low] * dhigh + source->GetY()[low + 1] * dlow) / (xvals[low + 1] - xvals[low]));
          }
        }
      };
    }
    else if (_source->InheritsFrom(TF1::Class())) {
      auto* source(static_cast<TF1 const*>(_source));

      reweight_ = [this, source](std::vector<double>& _values) {
        _values.clear();

        unsigned nD(this->reweightExpr_->GetNdata());

        for (unsigned iD(0); iD != nD; ++iD) {
          double x(this->reweightExpr_->EvalInstance(iD));

          _values.push_back(source->Eval(x));
        }
      };
    }
    else
      throw std::runtime_error("Incompatible object passed as reweight source");
  }
  else {
    reweight_ = [this](std::vector<double>& _values) {
      _values.clear();

      unsigned nD(this->reweightExpr_->GetNdata());

      for (unsigned iD(0); iD != nD; ++iD)
        _values.push_back(this->reweightExpr_->EvalInstance(iD));
    };
  }
}

multidraw::Plot1DFiller&
multidraw::MultiDraw::addPlot(TH1* _hist, char const* _expr, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/, Plot1DOverflowMode _overflowMode/* = kDefault*/)
{
  TTreeFormulaCached* exprFormula(library_.getFormula(_expr));

  auto newPlot1D([_hist, _overflowMode, exprFormula](TTreeFormula* _reweightFormula)->ExprFiller* {
      return new Plot1DFiller(*_hist, *exprFormula, _reweightFormula, _overflowMode);
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

multidraw::TreeFiller&
multidraw::MultiDraw::addTree(TTree* _tree, char const* _cutName/* = ""*/, char const* _reweight/* = ""*/)
{
  auto newTree([this, _tree](TTreeFormula* _reweightFormula)->ExprFiller* {
      return new TreeFiller(*_tree, &this->library_, _reweightFormula);
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

  TTreeFormulaCached* reweightFormula(nullptr);
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

  for (auto* cut : cuts_) {
    cut->setPrintLevel(printLevel_);
    cut->resetCount();
  }

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
  
  while (iEntry != iEntryMax) {
    long long iLocalEntry(tree_.LoadTree(iEntry));
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
      ff.second->ResetCache();

    if (weightBranch != nullptr) {
      weightBranch->GetEntry(iLocalEntry);

      if (printLevel_ > 3)
        std::cout << "        Input weight " << getWeight() << std::endl;
    }

    double commonWeight(getWeight() * constWeight_);

    if (reweight_) {
      reweight_(eventWeights);
      if (eventWeights.empty())
        continue;

      for (double& w : eventWeights)
        w *= commonWeight;
    }
    else
      eventWeights.assign(1, commonWeight);

    if (printLevel_ > 3) {
      std::cout << "         Global weights: ";
      for (double w : eventWeights)
        std::cout << w << " ";
      std::cout << std::endl;
    }

    for (auto* cut : cuts_)
      cut->evaluateAndFill(eventWeights);
  }

  totalEvents_ = iEntry;

  if (printLevel_ >= 0) {
    std::cout << "\r      " << iEntry << " events";
    std::cout << std::endl;
  }

  if (printLevel_ > 0) {
    for (auto* cut : cuts_) {
      std::cout << "        Cut " << cut->getName() << ": passed total " << cut->getCount() << std::endl;
      for (unsigned iF(0); iF != cut->getNFillers(); ++iF) {
        auto* filler(cut->getFiller(iF));
        std::cout << "          " << filler->getObj().GetName() << ": " << filler->getCount() << std::endl;
      }
    }
  }
}
