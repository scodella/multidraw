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
  sources_(_orig.sources_),
  printLevel_(_orig.printLevel_)
{
  if (_orig.reweightSource_)
    reweightSource_ = std::make_unique<ReweightSource>(*_orig.reweightSource_);
}

multidraw::ExprFiller::ExprFiller(TObject& _tobj, ExprFiller const& _orig) :
  tobj_(_tobj),
  sources_(_orig.sources_),
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
multidraw::ExprFiller::bindTree(FormulaLibrary& _formulaLibrary, FunctionLibrary& _functionLibrary)
{
  unlinkTree();

  for (auto& source : sources_) {
    if (std::strlen(source.getFormula()) != 0)
      compiledExprs_.emplace_back(new CompiledExpr(_formulaLibrary.getFormula(source.getFormula())));
    else
      compiledExprs_.emplace_back(new CompiledExpr(_functionLibrary.getFunction(*source.getFunction())));
  }

  if (reweightSource_)
    compiledReweight_ = reweightSource_->compile(_formulaLibrary);

  counter_ = 0;
}

void
multidraw::ExprFiller::unlinkTree()
{
  compiledExprs_.clear();
  compiledReweight_ = nullptr;
}

multidraw::ExprFillerPtr
multidraw::ExprFiller::threadClone(FormulaLibrary& _formulaLibrary, FunctionLibrary& _functionLibrary)
{
  ExprFillerPtr clone(clone_());
  clone->cloneSource_ = this;
  clone->setPrintLevel(-1);
  clone->bindTree(_formulaLibrary, _functionLibrary);
  return clone;
}

void
multidraw::ExprFiller::initialize()
{
  // Manage all dimensions with a single manager
  TTreeFormulaManager* manager(nullptr);
  for (auto& expr : compiledExprs_) {
    if (expr->getFormula() == nullptr)
      continue;

    if (manager == nullptr)
      manager = expr->getFormula()->GetManager();
    else
      manager->Add(expr->getFormula());
  }

  if (manager != nullptr)
    manager->Sync();
}

void
multidraw::ExprFiller::fill(std::vector<double> const& _eventWeights, std::vector<int> const& _categories)
{
  // All exprs and reweight exprs share the same manager
  unsigned nD(compiledExprs_.at(0)->getNdata());

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
        expr->getNdata();
        if (iD != 0) // need to always call EvalInstance(0)
          expr->evaluate(0);
      }
    }

    loaded = true;

    if (iD < _eventWeights.size())
      entryWeight_ = _eventWeights[iD];
    else
      entryWeight_ = _eventWeights.back();

    if (compiledReweight_ != nullptr)
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
