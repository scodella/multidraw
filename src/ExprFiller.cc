#include "../inc/ExprFiller.h"
#include "../inc/TTreeFormulaCached.h"

#include <iostream>

multidraw::ExprFiller::ExprFiller(TTreeFormula* _reweight/* = nullptr*/) :
  reweight_(_reweight)
{
}

multidraw::ExprFiller::ExprFiller(ExprFiller const& _orig) :
  ownFormulas_(_orig.ownFormulas_)
{
  if (ownFormulas_) {
    for (unsigned iD(0); iD != _orig.getNdim(); ++iD) {
      auto& oexpr(*_orig.getExpr(iD));
      auto* formula(NewTTreeFormula(oexpr.GetName(), oexpr.GetTitle(), oexpr.GetTree()));
      if (formula == nullptr)
        throw std::runtime_error("Failed to compile formula.");

      exprs_.push_back(formula);
    }

    if (_orig.reweight_ != nullptr) {
      reweight_ = NewTTreeFormula(_orig.reweight_->GetName(), _orig.reweight_->GetTitle(), _orig.reweight_->GetTree());
      if (reweight_ == nullptr)
        throw std::runtime_error("Failed to compile reweight.");
    }
  }
  else {
    exprs_ = _orig.exprs_;
    reweight_ = _orig.reweight_;
  }
}

multidraw::ExprFiller::~ExprFiller()
{
  if (ownFormulas_) {
    delete reweight_;

    for (auto* expr : exprs_)
      delete expr;
  }
}

void
multidraw::ExprFiller::updateTree()
{
  for (auto* expr : exprs_)
    expr->UpdateFormulaLeaves();

  if (reweight_ != nullptr)
    reweight_->UpdateFormulaLeaves();
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
