#include "../interface/Cut.h"
#include "../interface/ExprFiller.h"

#include "TTreeFormulaManager.h"

#include <iostream>

multidraw::Cut::Cut(char const* _name, char const* _expr/* = ""*/) :
  name_(_name),
  cutExpr_(_expr)
{
}

multidraw::Cut::~Cut()
{
  unlinkTree();

  for (auto* filler : fillers_) {
    filler->mergeBack();
    delete filler;
  }
}

TString
multidraw::Cut::getName() const
{
  if (name_.Length() == 0)
    return "[Event filter]";
  else
    return name_;
}

void
multidraw::Cut::setPrintLevel(int _l)
{
  printLevel_ = _l;
  for (auto* filler : fillers_)
    filler->setPrintLevel(_l);
}

void
multidraw::Cut::setCategorization(char const* _expr)
{
  categoryExprs_.clear();
  categorizationExpr_ = _expr;
}

int
multidraw::Cut::getNCategories() const
{
  if (categorizationExpr_.Length() != 0)
    return -1; // unknown
  else
    return categoryExprs_.size();
}

void
multidraw::Cut::bindTree(FormulaLibrary& _library)
{
  counter_ = 0;

  compiledCut_.reset();
  if (cutExpr_.Length() != 0)
    compiledCut_ = _library.getFormula(cutExpr_);

  if (categorizationExpr_.Length() != 0)
    compiledCategorization_ = _library.getFormula(categorizationExpr_);
  else {
    compiledCategories_.clear();
    for (auto& expr : categoryExprs_)
      compiledCategories_.push_back(_library.getFormula(expr));
  }

  for (auto* filler : fillers_)
    filler->bindTree(_library);
}

void
multidraw::Cut::unlinkTree()
{
  compiledCut_.reset();

  compiledCategorization_.reset();
  compiledCategories_.clear();

  for (auto* filler : fillers_)
    filler->unlinkTree();
}

multidraw::Cut*
multidraw::Cut::threadClone(FormulaLibrary& _library) const
{
  Cut* clone(new Cut(name_, cutExpr_));
  clone->setPrintLevel(-1);

  if (categorizationExpr_.Length() != 0)
    clone->setCategorization(categorizationExpr_);
  else {
    for (auto& expr : categoryExprs_)
      clone->addCategory(expr);
  }

  for (auto* filler : fillers_)
    clone->addFiller(*filler->threadClone(_library));

  clone->bindTree(_library);

  return clone;
}

bool
multidraw::Cut::cutDependsOn(TTree const* _tree) const
{
  if (!compiledCut_)
    return false;

  unsigned iL(0);
  while (true) {
    auto* leaf(compiledCut_->GetLeaf(iL++));
    if (leaf == nullptr)
      break;

    if (leaf->GetBranch()->GetTree() == _tree)
      return true;
  }

  if (compiledCategorization_) {
    iL = 0;
    while (true) {
      auto* leaf(compiledCategorization_->GetLeaf(iL++));
      if (leaf == nullptr)
        break;

      if (leaf->GetBranch()->GetTree() == _tree)
        return true;
    }
  }

  for (auto& cat : compiledCategories_) {
    iL = 0;
    while (true) {
      auto* leaf(cat->GetLeaf(iL++));
      if (leaf == nullptr)
        break;

      if (leaf->GetBranch()->GetTree() == _tree)
        return true;
    }
  }

  return false;
}

void
multidraw::Cut::initialize()
{
  if (!compiledCut_)
    return;

  // Each formula object has a default manager
  auto* formulaManager(compiledCut_->GetManager());
  if (compiledCategorization_)
    formulaManager->Add(compiledCategorization_.get());
  else {
    for (auto& cat : compiledCategories_)
      formulaManager->Add(cat.get());
  }

  formulaManager->Sync();

  for (auto* filler : fillers_)
    filler->initialize();
}

bool
multidraw::Cut::evaluate()
{
  if (!compiledCut_)
    return true;

  unsigned nD(compiledCut_->GetNdata());
  if (compiledCategorization_) {
    compiledCategorization_->GetNdata();
    compiledCategorization_->EvalInstance(0);
  }
  else {
    for (auto& cat : compiledCategories_) {
      cat->GetNdata();
      cat->EvalInstance(0);
    }
  }

  categoryIndex_.assign(nD, -1);

  if (printLevel_ > 2)
    std::cout << "        " << getName() << " has " << nD << " iterations" << std::endl;

  bool any(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (compiledCut_->EvalInstance(iD) == 0.)
      continue;

    if (compiledCategorization_)
      categoryIndex_[iD] = int(compiledCategorization_->EvalInstance(iD));
    else if (!compiledCategories_.empty()) {
      for (unsigned icat(0); icat != compiledCategories_.size(); ++icat) {
        if (compiledCategories_[icat]->EvalInstance(iD) != 0.) {
          categoryIndex_[iD] = icat;
          break;
        }
      }
    }
    else
      categoryIndex_[iD] = 0;

    any = true;

    if (printLevel_ > 2)
      std::cout << "        " << getName() << " iteration " << iD << " pass" << std::endl;
  }
  
  return any;
}

void
multidraw::Cut::fillExprs(std::vector<double> const& _eventWeights)
{
  ++counter_;

  for (auto* filler : fillers_)
    filler->fill(_eventWeights, categoryIndex_);
}
