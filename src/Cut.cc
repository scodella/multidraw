#include "../interface/Cut.h"
#include "../interface/ExprFiller.h"

#include <iostream>

multidraw::Cut::Cut(char const* _name, TTreeFormulaCachedPtr const& _formula) :
  name_(_name)
{
  setFormula(_formula);
}

multidraw::Cut::Cut(char const* _name, Evaluable::InstanceVal const& _inst, Evaluable::NData const& _ndata/* = nullptr*/) :
  name_(_name)
{
  setFormula(_inst, _ndata);
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
  cut_.set(_formula);

  delete instanceMask_;
  instanceMask_ = nullptr;
  if (!cut_.singleValue())
    instanceMask_ = new std::vector<bool>;
}

void
multidraw::Cut::setFormula(Evaluable::InstanceVal const& _inst, Evaluable::NData const& _ndata/* = nullptr*/)
{
  cut_.set(_inst, _ndata);

  delete instanceMask_;
  instanceMask_ = nullptr;
  if (!cut_.singleValue())
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
  unsigned nD(cut_.getNdata());

  if (!cut_.singleValue())
    instanceMask_->assign(nD, false);

  if (printLevel_ > 2)
    std::cout << "        " << getName() << " has " << nD << " iterations" << std::endl;

  bool any(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (cut_.evalInstance(iD) == 0.)
      continue;

    any = true;

    if (printLevel_ > 2)
      std::cout << "        " << getName() << " iteration " << iD << " pass" << std::endl;

    if (!cut_.singleValue())
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
