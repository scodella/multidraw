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
  cuts_.emplace("", new Cut("", []()->bool { return true; }));
}

multidraw::MultiDraw::~MultiDraw()
{
  for (auto& p : cuts_)
    delete p.second;
}

void
multidraw::MultiDraw::setFilter(char const* _expr)
{
  cuts_[""]->setFormula(*library_.getFormula(_expr));
}

void
multidraw::MultiDraw::addCut(char const* _name, char const* _expr)
{
  if (cuts_.count(_name) != 0) {
    std::stringstream ss;
    ss << "Cut named " << _name << " already exists";
    std::cout << ss.str() << std::endl;
    throw std::invalid_argument(ss.str());
  }

  cuts_.emplace(_name, new Cut(_name, *library_.getFormula(_expr)));
}

void
multidraw::MultiDraw::setPrescale(unsigned _p, char const* _evtNumBranch/* = ""*/)
{
  prescale_ = _p;
  evtNumBranch_ = _evtNumBranch;
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
multidraw::MultiDraw::addObj_(char const* _cutName, char const* _reweight, ObjGen const& _gen)
{
  auto cutItr(cuts_.find(_cutName));
  if (cutItr == cuts_.end()) {
    std::stringstream ss;
    ss << "Cut \"" << _cutName << "\" not defined";
    std::cerr << ss.str() << std::endl;
    throw std::runtime_error(ss.str());
  }

  auto& cut(*cutItr->second);

  TTreeFormulaCached* reweightFormula(nullptr);
  if (_reweight != nullptr && std::strlen(_reweight) != 0)
    reweightFormula = library_.getFormula(_reweight);

  auto* filler(_gen(reweightFormula));

  cut.addFiller(*filler);

  return *filler;
}

void
multidraw::MultiDraw::execute(long _nEntries/* = -1*/, long _firstEntry/* = 0*/)
{
  // float* weightF(nullptr);
  // double weight(1.);
  // unsigned eventNumber;
  // TBranch* weightBranch(nullptr);
  // TBranch* eventNumberBranch(nullptr);

  // for (auto* plots : {&postFull_, &postBase_, &unconditional_}) {
  //   for (auto* plot : *plots) {
  //     plot->setPrintLevel(printLevel_);
  //     plot->resetCount();
  //   }
  // }

  // std::vector<double> eventWeights;
  // std::vector<bool>* baseResults(nullptr);
  // std::vector<bool>* fullResults(nullptr);

  // if (baseSelection_ && baseSelection_->GetMultiplicity() != 0) {
  //   if (printLevel_ > 1)
  //     std::cout << "\n\nBase selection is based on an array." << std::endl;

  //   baseResults = new std::vector<bool>;
  // }
  // if (fullSelection_ && fullSelection_->GetMultiplicity() != 0) {
  //   if (printLevel_ > 1)
  //     std::cout << "\nFull selection is based on an array." << std::endl;

  //   fullResults = new std::vector<bool>;
  // }

  // long printEvery(10000);
  // if (printLevel_ == 2)
  //   printEvery = 10000;
  // else if (printLevel_ == 3)
  //   printEvery = 100;
  // else if (printLevel_ >= 4)
  //   printEvery = 1;

  // if (_filter != nullptr && std::strlen(_filter) != 0) {
  //   tree_.Draw(">>elist", _filter, "entrylist");
  //   auto* elist(static_cast<TEntryList*>(gDirectory->Get("elist")));
  //   tree_.SetEntryList(elist);
  // }

  // long long iEntry(_firstEntry);
  // long long iEntryMax(_firstEntry + _nEntries);
  // int treeNumber(-1);
  // unsigned passBase(0);
  // unsigned passFull(0);
  // while (iEntry != iEntryMax) {
  //   long long iEntryFiltered(tree_.GetEntryNumber(iEntry++));
  //   if (iEntryFiltered < 0)
  //     break;
  //   long long iLocalEntry(tree_.LoadTree(iEntryFiltered));
  //   if (iLocalEntry < 0)
  //     break;

  //   if (printLevel_ >= 0 && iEntry % printEvery == 1) {
  //     std::cout << "\r      " << iEntry << " events";
  //     if (printLevel_ > 2)
  //       std::cout << std::endl;
  //   }

  //   if (treeNumber != tree_.GetTreeNumber()) {
  //     if (printLevel_ > 1)
  //       std::cout << "      Opened a new file: " << tree_.GetCurrentFile()->GetName() << std::endl;

  //     treeNumber = tree_.GetTreeNumber();

  //     if (weightBranchName_.Length() != 0) {
  //       weightBranch = tree_.GetBranch(weightBranchName_);
  //       if (!weightBranch)
  //         throw std::runtime_error(("Could not find branch " + weightBranchName_).Data());

  //       auto* leaves(weightBranch->GetListOfLeaves());
  //       if (leaves->GetEntries() == 0) // ??
  //         throw std::runtime_error(("Branch " + weightBranchName_ + " does not have any leaves").Data());

  //       auto* leaf(static_cast<TLeaf*>(leaves->At(0)));

  //       if (leaf->InheritsFrom(TLeafF::Class())) {
  //         if (weightF == nullptr)
  //           weightF = new float;
  //         weightBranch->SetAddress(weightF);
  //       }
  //       else if (leaf->InheritsFrom(TLeafD::Class())) {
  //         if (weightF != nullptr) {
  //           // we don't really need to handle a case like this (trees with different weight branch types are mixed), but we can..
  //           delete weightF;
  //           weightF = nullptr;
  //         }
  //         weightBranch->SetAddress(&weight);
  //       }
  //       else
  //         throw std::runtime_error(("I do not know how to read the leaf type of branch " + weightBranchName_).Data());
  //     }

  //     if (prescale_ > 1) {
  //       eventNumberBranch = tree_.GetBranch("eventNumber");
  //       if (!eventNumberBranch)
  //         throw std::runtime_error("Event number not available");

  //       eventNumberBranch->SetAddress(&eventNumber);
  //     }

  //     for (auto& ff : library_)
  //       ff.second->UpdateFormulaLeaves();
  //   }

  //   if (prescale_ > 1) {
  //     eventNumberBranch->GetEntry(iLocalEntry);

  //     if (eventNumber % prescale_ != 0)
  //       continue;
  //   }

  //   // Reset formula cache
  //   for (auto& ff : library_)
  //     ff.second->ResetCache();

  //   if (weightBranch) {
  //     weightBranch->GetEntry(iLocalEntry);
  //     if (weightF != nullptr)
  //       weight = *weightF;

  //     if (printLevel_ > 3)
  //       std::cout << "        Input weight " << weight << std::endl;
  //   }

  //   if (reweight_) {
  //     reweight_(eventWeights);
  //     if (eventWeights.empty())
  //       continue;

  //     for (double& w : eventWeights)
  //       w *= weight * constWeight_;
  //   }
  //   else
  //     eventWeights.assign(1, weight * constWeight_);

  //   if (printLevel_ > 3) {
  //     std::cout << "         Global weights: ";
  //     for (double w : eventWeights)
  //       std::cout << w << " ";
  //     std::cout << std::endl;
  //   }    

  //   // Plots that do not require passing the baseline cut
  //   for (auto* plot : unconditional_) {
  //     if (printLevel_ > 3)
  //       std::cout << "        Filling " << plot->getObj()->GetName() << std::endl;

  //     plot->fill(eventWeights);
  //   }

  //   // Baseline cut
  //   if (baseSelection_) {
  //     unsigned nD(baseSelection_->GetNdata());

  //     if (printLevel_ > 2)
  //       std::cout << "        Base selection has " << nD << " iterations" << std::endl;

  //     bool any(false);

  //     if (baseResults)
  //       baseResults->assign(nD, false);

  //     for (unsigned iD(0); iD != nD; ++iD) {
  //       if (baseSelection_->EvalInstance(iD) != 0.) {
  //         any = true;

  //         if (printLevel_ > 2)
  //           std::cout << "        Base selection " << iD << " is true" << std::endl;

  //         if (baseResults)
  //           (*baseResults)[iD] = true;
  //         else
  //           break; // no need to evaluate more
  //       }
  //     }

  //     if (!any)
  //       continue;
  //   }

  //   ++passBase;

  //   // Plots that require passing the baseline cut but not the full cut
  //   for (auto* plot : postBase_) {
  //     if (printLevel_ > 3)
  //       std::cout << "        Filling " << plot->getObj()->GetName() << std::endl;

  //     plot->fill(eventWeights, baseResults);
  //   }

  //   // Full cut
  //   if (fullSelection_) {
  //     unsigned nD(fullSelection_->GetNdata());

  //     if (printLevel_ > 2)
  //       std::cout << "        Full selection has " << nD << " iterations" << std::endl;

  //     bool any(false);

  //     if (fullResults)
  //       fullResults->assign(nD, false);

  //     // fullResults for iD >= baseResults->size() will never be true
  //     if (baseResults && baseResults->size() < nD)
  //       nD = baseResults->size();

  //     bool loaded(false);

  //     for (unsigned iD(0); iD != nD; ++iD) {
  //       if (baseResults && !(*baseResults)[iD])
  //         continue;

  //       if (!loaded && iD != 0)
  //         fullSelection_->EvalInstance(0);

  //       loaded = true;

  //       if (fullSelection_->EvalInstance(iD) != 0.) {
  //         any = true;

  //         if (printLevel_ > 2)
  //           std::cout << "        Full selection " << iD << " is true" << std::endl;

  //         if (fullResults)
  //           (*fullResults)[iD] = true;
  //         else
  //           break;
  //       }
  //     }

  //     if (!any)
  //       continue;
  //   }

  //   ++passFull;

  //   // Plots that require all cuts
  //   for (auto* plot : postFull_) {
  //     if (printLevel_ > 3)
  //       std::cout << "        Filling " << plot->getObj()->GetName() << std::endl;

  //     plot->fill(eventWeights, fullResults);
  //   }
  // }

  // if (tree_.GetEntryList() != nullptr) {
  //   auto* elist(tree_.GetEntryList());
  //   tree_.SetEntryList(nullptr);
  //   delete elist;
  // }

  // delete baseResults;
  // delete fullResults;
  // delete weightF;

  // totalEvents_ = iEntry;

  // if (printLevel_ >= 0) {
  //   std::cout << "\r      " << iEntry << " events";
  //   std::cout << std::endl;
  // }

  // if (printLevel_ > 0) {
  //   std::cout << "      " << passBase << " passed base selection" << std::endl;
  //   std::cout << "      " << passFull << " passed full selection" << std::endl;

  //   for (auto* plots : {&postFull_, &postBase_, &unconditional_}) {
  //     for (auto* plot : *plots)
  //       std::cout << "        " << plot->getObj()->GetName() << ": " << plot->getCount() << std::endl;
  //   }
  // }
}
