#include "../interface/ExprFiller.h"

#include <iostream>

multidraw::ExprFiller::ExprFiller(Reweight const& _reweight) :
  reweight_(_reweight)
{
}

multidraw::ExprFiller::ExprFiller(ExprFiller const& _orig) :
  exprs_(_orig.exprs_),
  reweight_(_orig.reweight_),
  printLevel_(_orig.printLevel_)
{
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
      for (auto& expr : exprs_) {
        expr->GetNdata();
        if (iD != 0) // need to always call EvalInstance(0)
          expr->EvalInstance(0);
      }
    }

    loaded = true;

    if (iD < _eventWeights.size())
      entryWeight_ = _eventWeights[iD];
    else
      entryWeight_ = _eventWeights.back();

    entryWeight_ *= reweight_.evaluate(iD);

    doFill_(iD);
  }
}
