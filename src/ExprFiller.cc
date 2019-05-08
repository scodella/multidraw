#include "../interface/ExprFiller.h"

#include "TTree.h"
#include "TTreeFormulaManager.h"

#include <iostream>
#include <cstring>

multidraw::ExprFiller::ExprFiller(TObject& _tobj, char const* _reweight/* = ""*/) :
  tobj_(_tobj)
{
  if (_reweight != nullptr && std::strlen(_reweight) != 0)
    reweightSource_ = std::make_unique<ReweightSource>(_reweight);
}

multidraw::ExprFiller::ExprFiller(ExprFiller const& _orig) :
  tobj_(_orig.tobj_),
  exprs_(_orig.exprs_),
  printLevel_(_orig.printLevel_)
{
  if (_orig.reweightSource_)
    reweightSource_ = std::make_unique<ReweightSource>(*_orig.reweightSource_);
}

multidraw::ExprFiller::ExprFiller(TObject& _tobj, ExprFiller const& _orig) :
  tobj_(_tobj),
  exprs_(_orig.exprs_),
  printLevel_(_orig.printLevel_)
{
  if (_orig.reweightSource_)
    reweightSource_ = std::make_unique<ReweightSource>(*_orig.reweightSource_);
}

multidraw::ExprFiller::~ExprFiller()
{
  unlinkTree();

  if (cloneSource_ != nullptr)
    delete &tobj_;
}

void
multidraw::ExprFiller::bindTree(FormulaLibrary& _library)
{
  compiledExprs_.clear();

  for (auto& expr : exprs_)
    compiledExprs_.push_back(_library.getFormula(expr));

  if (reweightSource_)
    compiledReweight_ = reweightSource_->compile(_library);
  else
    compiledReweight_ = nullptr;

  counter_ = 0;
}

void
multidraw::ExprFiller::unlinkTree()
{
  compiledExprs_.clear();

  compiledReweight_ = nullptr;
}

multidraw::ExprFiller*
multidraw::ExprFiller::threadClone(FormulaLibrary& _library)
{
  auto* clone(clone_());
  clone->cloneSource_ = this;
  clone->setPrintLevel(-1);
  clone->bindTree(_library);
  return clone;
}

void
multidraw::ExprFiller::initialize()
{
  if (compiledExprs_.empty()) // cannot be
    return;

  // Manage all dimensions with a single manager
  auto* manager(compiledExprs_.at(0)->GetManager());
  for (unsigned iE(1); iE != compiledExprs_.size(); ++iE)
    manager->Add(compiledExprs_[iE].get());

  manager->Sync();
}

void
multidraw::ExprFiller::fill(std::vector<double> const& _eventWeights, std::vector<int> const& _categories)
{
  // All exprs and reweight exprs share the same manager
  unsigned nD(compiledExprs_.at(0)->GetNdata());

  if (printLevel_ > 3)
    std::cout << "          " << getObj().GetName() << "::fill() => " << nD << " iterations" << std::endl;

  if (_categories.size() < nD)
    nD = _categories.size();

  bool loaded(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (_categories[iD] < 0)
      continue;

    ++counter_;

    if (!loaded) {
      for (auto& expr : compiledExprs_) {
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

    if (compiledReweight_)
      entryWeight_ *= compiledReweight_->evaluate(iD);

    doFill_(iD, _categories[iD]);
  }
}

void
multidraw::ExprFiller::mergeBack()
{
  if (cloneSource_ == nullptr)
    return;

  mergeBack_();
}
