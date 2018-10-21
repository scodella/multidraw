#include "../inc/MultiDraw.h"
#include "../inc/Plot1DFiller.h"
#include "../inc/TreeFiller.h"

#include "TFile.h"
#include "TBranch.h"
#include "TGraph.h"
#include "TF1.h"
#include "TError.h"
#include "TLeafF.h"
#include "TLeafD.h"
#include "TEntryList.h"

#include <stdexcept>
#include <cstring>
#include <iostream>
#include <algorithm>

multidraw::MultiDraw::MultiDraw(char const* _treeName/* = "events"*/) :
  tree_(_treeName)
{
}

multidraw::MultiDraw::~MultiDraw()
{
  for (auto* plots : {&postFull_, &postBase_, &unconditional_}) {
    for (auto* plot : *plots)
      delete plot;
  }

  for (auto& ff : library_)
    delete ff.second;
}

void
multidraw::MultiDraw::setBaseSelection(char const* _cuts)
{
  if (baseSelection_ != nullptr) {
    deleteFormula_(baseSelection_);
    baseSelection_ = nullptr;
  }

  if (!_cuts || std::strlen(_cuts) == 0)
    return;

  baseSelection_ = getFormula_(_cuts);
  if (baseSelection_ == nullptr)
    std::cerr << "Failed to compile base selection " << _cuts << std::endl;
}

void
multidraw::MultiDraw::setFullSelection(char const* _cuts)
{
  if (fullSelection_ != nullptr) {
    deleteFormula_(fullSelection_);
    fullSelection_ = nullptr;
  }

  if (!_cuts || std::strlen(_cuts) == 0)
    return;

  fullSelection_ = getFormula_(_cuts);
  if (fullSelection_ == nullptr)
    std::cerr << "Failed to compile full selection " << _cuts << std::endl;
}

void
multidraw::MultiDraw::setReweight(char const* _expr, TObject const* _source/* = nullptr*/)
{
  if (reweightExpr_ != nullptr) {
    deleteFormula_(reweightExpr_);
    reweightExpr_ = nullptr;
  }

  reweight_ = nullptr;

  if (!_expr || std::strlen(_expr) == 0)
    return;

  reweightExpr_ = getFormula_(_expr);
  if (reweightExpr_ == nullptr) {
    std::cerr << "Failed to compile reweight expression " << _expr << std::endl;
    return;
  }

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

void
multidraw::MultiDraw::addPlot(TH1* _hist, char const* _expr, char const* _cuts/* = ""*/, bool _applyBaseline/* = true*/, bool _applyFullSelection/* = false*/, char const* _reweight/* = ""*/, Plot1DOverflowMode _overflowMode/* = kDefault*/)
{
  TTreeFormulaCached* exprFormula(getFormula_(_expr));
  if (exprFormula == nullptr) {
    std::cerr << "Plot " << _hist->GetName() << " cannot be added (invalid expression)" << std::endl;
    return;
  }

  auto newPlot([_hist, _overflowMode, exprFormula](TTreeFormula* _cutsFormula, TTreeFormula* _reweightFormula)->ExprFiller* {
      return new Plot1DFiller(*_hist, *exprFormula, _cutsFormula, _reweightFormula, _overflowMode);
    });

  if (printLevel_ > 1) {
    std::cout << "\nAdding Plot " << _hist->GetName() << " with expression " << _expr << std::endl;
    if (_cuts != nullptr && std::strlen(_cuts) != 0)
      std::cout << " Cuts: " << _cuts << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  addObj_(_cuts, _applyBaseline, _applyFullSelection, _reweight, newPlot);
}

void
multidraw::MultiDraw::addTree(TTree* _tree, char const* _cuts/* = ""*/, bool _applyBaseline/* = true*/, bool _applyFullSelection/* = false*/, char const* _reweight/* = ""*/)
{
  auto newTree([_tree](TTreeFormula* _cutsFormula, TTreeFormula* _reweightFormula)->ExprFiller* {
      return new TreeFiller(*_tree, _cutsFormula, _reweightFormula);
    });

  if (printLevel_ > 1) {
    std::cout << "\nAdding Tree " << _tree->GetName() << std::endl;
    if (_cuts != nullptr && std::strlen(_cuts) != 0)
      std::cout << " Cuts: " << _cuts << std::endl;
    if (_reweight != nullptr && std::strlen(_reweight) != 0)
      std::cout << " Reweight: " << _reweight << std::endl;
  }

  addObj_(_cuts, _applyBaseline, _applyFullSelection, _reweight, newTree);
}

void
multidraw::MultiDraw::addTreeBranch(TTree* _tree, char const* _bname, char const* _expr)
{
  auto* exprFormula(getFormula_(_expr));
  if (exprFormula == nullptr) {
    std::cerr << "Branch " << _bname << " cannot be added (invalid expression)" << std::endl;
    return;
  }

  for (auto* plots : {&postFull_, &postBase_, &unconditional_}) {
    for (auto* plot : *plots) {
      if (plot->getObj() == _tree) {
        if (printLevel_ > 1)
          std::cout << "Adding a branch " << _bname << " to tree " << plot->getObj()->GetName() << " with expression " << _expr << std::endl;

        static_cast<TreeFiller*>(plot)->addBranch(_bname, *exprFormula);
      }
    }
  }
}

void
multidraw::MultiDraw::addObj_(char const* _cuts, bool _applyBaseline, bool _applyFullSelection, char const* _reweight, ObjGen const& _gen)
{
  TTreeFormulaCached* cutsFormula(nullptr);
  if (_cuts != nullptr && std::strlen(_cuts) != 0) {
    cutsFormula = getFormula_(_cuts);
    if (cutsFormula == nullptr) {
      std::cerr << "Failed to compile cuts " << _cuts << std::endl;
      return;
    }
  }

  TTreeFormulaCached* reweightFormula(nullptr);
  if (_reweight != nullptr && std::strlen(_reweight) != 0) {
    reweightFormula = getFormula_(_reweight);
    if (reweightFormula == nullptr) {
      std::cerr << "Failed to compile reweight " << _reweight << std::endl;
      return;
    }
  }

  std::vector<ExprFiller*>* stack(nullptr);
  if (_applyBaseline) {
    if (_applyFullSelection)
      stack = &postFull_;
    else
      stack = &postBase_;
  }
  else
    stack = &unconditional_;

  stack->push_back(_gen(cutsFormula, reweightFormula));
}

TTreeFormulaCached*
multidraw::MultiDraw::getFormula_(char const* _expr)
{
  auto fItr(library_.find(_expr));
  if (fItr != library_.end()) {
    fItr->second->SetNRef(fItr->second->GetNRef() + 1);
    return fItr->second;
  }

  auto* f(NewTTreeFormulaCached("formula", _expr, &tree_));
  if (f == nullptr)
    return nullptr;

  library_.emplace(_expr, f);

  return f;
}

void
multidraw::MultiDraw::deleteFormula_(TTreeFormulaCached* _formula)
{
  if (_formula->GetNRef() == 1) {
    library_.erase(_formula->GetTitle());
    delete _formula;
  }
}

void
multidraw::MultiDraw::fillPlots(long _nEntries/* = -1*/, long _firstEntry/* = 0*/, char const* _filter/* = nullptr*/)
{
  float* weightF(nullptr);
  double weight(1.);
  unsigned eventNumber;
  TBranch* weightBranch(nullptr);
  TBranch* eventNumberBranch(nullptr);

  for (auto* plots : {&postFull_, &postBase_, &unconditional_}) {
    for (auto* plot : *plots) {
      plot->setPrintLevel(printLevel_);
      plot->resetCount();
    }
  }

  std::vector<double> eventWeights;
  std::vector<bool>* baseResults(nullptr);
  std::vector<bool>* fullResults(nullptr);

  if (baseSelection_ && baseSelection_->GetMultiplicity() != 0) {
    if (printLevel_ > 1)
      std::cout << "\n\nBase selection is based on an array." << std::endl;

    baseResults = new std::vector<bool>;
  }
  if (fullSelection_ && fullSelection_->GetMultiplicity() != 0) {
    if (printLevel_ > 1)
      std::cout << "\nFull selection is based on an array." << std::endl;

    fullResults = new std::vector<bool>;
  }

  long printEvery(10000);
  if (printLevel_ == 2)
    printEvery = 10000;
  else if (printLevel_ == 3)
    printEvery = 100;
  else if (printLevel_ >= 4)
    printEvery = 1;

  if (_filter != nullptr && std::strlen(_filter) != 0) {
    tree_.Draw(">>elist", _filter, "entrylist");
    auto* elist(static_cast<TEntryList*>(gDirectory->Get("elist")));
    tree_.SetEntryList(elist);
  }

  long long iEntry(_firstEntry);
  long long iEntryMax(_firstEntry + _nEntries);
  int treeNumber(-1);
  unsigned passBase(0);
  unsigned passFull(0);
  while (iEntry != iEntryMax) {
    long long iEntryFiltered(tree_.GetEntryNumber(iEntry++));
    if (iEntryFiltered < 0)
      break;
    long long iLocalEntry(tree_.LoadTree(iEntryFiltered));
    if (iLocalEntry < 0)
      break;

    if (printLevel_ >= 0 && iEntry % printEvery == 1) {
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
        if (leaves->GetEntries() == 0) // ??
          throw std::runtime_error(("Branch " + weightBranchName_ + " does not have any leaves").Data());

        auto* leaf(static_cast<TLeaf*>(leaves->At(0)));

        if (leaf->InheritsFrom(TLeafF::Class())) {
          if (weightF == nullptr)
            weightF = new float;
          weightBranch->SetAddress(weightF);
        }
        else if (leaf->InheritsFrom(TLeafD::Class())) {
          if (weightF != nullptr) {
            // we don't really need to handle a case like this (trees with different weight branch types are mixed), but we can..
            delete weightF;
            weightF = nullptr;
          }
          weightBranch->SetAddress(&weight);
        }
        else
          throw std::runtime_error(("I do not know how to read the leaf type of branch " + weightBranchName_).Data());
      }

      if (prescale_ > 1) {
        eventNumberBranch = tree_.GetBranch("eventNumber");
        if (!eventNumberBranch)
          throw std::runtime_error("Event number not available");

        eventNumberBranch->SetAddress(&eventNumber);
      }

      for (auto& ff : library_)
        ff.second->UpdateFormulaLeaves();
    }

    if (prescale_ > 1) {
      eventNumberBranch->GetEntry(iLocalEntry);

      if (eventNumber % prescale_ != 0)
        continue;
    }

    // Reset formula cache
    for (auto& ff : library_)
      ff.second->ResetCache();

    if (weightBranch) {
      weightBranch->GetEntry(iLocalEntry);
      if (weightF != nullptr)
        weight = *weightF;

      if (printLevel_ > 3)
        std::cout << "        Input weight " << weight << std::endl;
    }

    if (reweight_) {
      reweight_(eventWeights);
      if (eventWeights.empty())
        continue;

      for (double& w : eventWeights)
        w *= weight * constWeight_;
    }
    else
      eventWeights.assign(1, weight * constWeight_);

    if (printLevel_ > 3) {
      std::cout << "         Global weights: ";
      for (double w : eventWeights)
        std::cout << w << " ";
      std::cout << std::endl;
    }    

    // Plots that do not require passing the baseline cut
    for (auto* plot : unconditional_) {
      if (printLevel_ > 3)
        std::cout << "        Filling " << plot->getObj()->GetName() << std::endl;

      plot->fill(eventWeights);
    }

    // Baseline cut
    if (baseSelection_) {
      unsigned nD(baseSelection_->GetNdata());

      if (printLevel_ > 2)
        std::cout << "        Base selection has " << nD << " iterations" << std::endl;

      bool any(false);

      if (baseResults)
        baseResults->assign(nD, false);

      for (unsigned iD(0); iD != nD; ++iD) {
        if (baseSelection_->EvalInstance(iD) != 0.) {
          any = true;

          if (printLevel_ > 2)
            std::cout << "        Base selection " << iD << " is true" << std::endl;

          if (baseResults)
            (*baseResults)[iD] = true;
          else
            break; // no need to evaluate more
        }
      }

      if (!any)
        continue;
    }

    ++passBase;

    // Plots that require passing the baseline cut but not the full cut
    for (auto* plot : postBase_) {
      if (printLevel_ > 3)
        std::cout << "        Filling " << plot->getObj()->GetName() << std::endl;

      plot->fill(eventWeights, baseResults);
    }

    // Full cut
    if (fullSelection_) {
      unsigned nD(fullSelection_->GetNdata());

      if (printLevel_ > 2)
        std::cout << "        Full selection has " << nD << " iterations" << std::endl;

      bool any(false);

      if (fullResults)
        fullResults->assign(nD, false);

      // fullResults for iD >= baseResults->size() will never be true
      if (baseResults && baseResults->size() < nD)
        nD = baseResults->size();

      bool loaded(false);

      for (unsigned iD(0); iD != nD; ++iD) {
        if (baseResults && !(*baseResults)[iD])
          continue;

        if (!loaded && iD != 0)
          fullSelection_->EvalInstance(0);

        loaded = true;

        if (fullSelection_->EvalInstance(iD) != 0.) {
          any = true;

          if (printLevel_ > 2)
            std::cout << "        Full selection " << iD << " is true" << std::endl;

          if (fullResults)
            (*fullResults)[iD] = true;
          else
            break;
        }
      }

      if (!any)
        continue;
    }

    ++passFull;

    // Plots that require all cuts
    for (auto* plot : postFull_) {
      if (printLevel_ > 3)
        std::cout << "        Filling " << plot->getObj()->GetName() << std::endl;

      plot->fill(eventWeights, fullResults);
    }
  }

  if (tree_.GetEntryList() != nullptr) {
    auto* elist(tree_.GetEntryList());
    tree_.SetEntryList(nullptr);
    delete elist;
  }

  delete baseResults;
  delete fullResults;
  delete weightF;

  totalEvents_ = iEntry;

  if (printLevel_ >= 0) {
    std::cout << "\r      " << iEntry << " events";
    std::cout << std::endl;
  }

  if (printLevel_ > 0) {
    std::cout << "      " << passBase << " passed base selection" << std::endl;
    std::cout << "      " << passFull << " passed full selection" << std::endl;

    for (auto* plots : {&postFull_, &postBase_, &unconditional_}) {
      for (auto* plot : *plots)
        std::cout << "        " << plot->getObj()->GetName() << ": " << plot->getCount() << std::endl;
    }
  }
}
