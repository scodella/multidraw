#include "../interface/Cut.h"
#include "../interface/ExprFiller.h"

#include <iostream>

multidraw::Cut::Cut(char const* _name, TTreeFormulaCachedPtr const& _formula/* = nullptr*/) :
  name_(_name)
{
  if (_formula)
    setFormula(_formula);
}

multidraw::Cut::~Cut()
{
  for (auto* filler : fillers_)
    delete filler;
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
multidraw::Cut::setFormula(TTreeFormulaCachedPtr const& _formula)
{
  cut_ = _formula;

  delete instanceMask_;
  instanceMask_ = nullptr;
  if (cut_->GetMultiplicity() != 0)
    instanceMask_ = new std::vector<bool>;
}

void
multidraw::Cut::setPrintLevel(int _l)
{
  printLevel_ = _l;
  for (auto* filler : fillers_)
    filler->setPrintLevel(_l);
}

void
multidraw::Cut::resetCount()
{
  counter_ = 0;
  for (auto* filler : fillers_)
    filler->resetCount();
}

bool
multidraw::Cut::evaluate()
{
  if (!cut_)
    return true;

  unsigned nD(cut_->GetNdata());

  if (cut_->GetMultiplicity() != 0)
    instanceMask_->assign(nD, false);

  if (printLevel_ > 2)
    std::cout << "        " << getName() << " has " << nD << " iterations" << std::endl;

  bool any(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (cut_->EvalInstance(iD) == 0.)
      continue;

    any = true;

    if (printLevel_ > 2)
      std::cout << "        " << getName() << " iteration " << iD << " pass" << std::endl;

    if (cut_->GetMultiplicity() != 0)
      (*instanceMask_)[iD] = true;
  }
  
  return any;
}

void
multidraw::Cut::fillExprs(std::vector<double> const& _eventWeights)
{
  ++counter_;

  for (auto* filler : fillers_)
    filler->fill(_eventWeights, instanceMask_);
}
