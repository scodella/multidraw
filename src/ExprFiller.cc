#include "../interface/ExprFiller.h"
#include "../interface/TTreeFormulaCached.h"

#include <iostream>

multidraw::ExprFiller::ExprFiller(TTreeFormula* _reweight/* = nullptr*/) :
  reweight_(_reweight)
{
}

multidraw::ExprFiller::ExprFiller(ExprFiller const& _orig)
{
  exprs_ = _orig.exprs_;
  reweight_ = _orig.reweight_;
}

void
multidraw::ExprFiller::fill(std::vector<double> const& _eventWeights, std::vector<bool> const* _presel/* = nullptr*/)
{
  // using the first expr for the number of instances
  unsigned nD(exprs_.at(0)->GetNdata());
  // need to call GetNdata before EvalInstance
  if (printLevel_ > 3)
    std::cout << "          " << getObj().GetName() << "::fill() => " << nD << " iterations" << std::endl;

  if (_presel != nullptr && _presel->size() < nD)
    nD = _presel->size();

  bool loaded(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (_presel != nullptr && !(*_presel)[iD])
      continue;

    ++counter_;

    if (!loaded) {
      for (unsigned iE(0); iE != exprs_.size(); ++iE) {
        exprs_[iE]->GetNdata();
        if (iD != 0) // need to always call EvalInstance(0)
          exprs_[iE]->EvalInstance(0);
      }
      if (reweight_ != nullptr) {
        reweight_->GetNdata();
        if (iD != 0)
          reweight_->EvalInstance(0);
      }
    }

    loaded = true;

    if (iD < _eventWeights.size())
      entryWeight_ = _eventWeights[iD];
    else
      entryWeight_ = _eventWeights.back();

    if (reweight_ != nullptr)
      entryWeight_ *= reweight_->EvalInstance(iD);

    doFill_(iD);
  }
}
