#include "../interface/Cut.h"
#include "../interface/ExprFiller.h"

#include <iostream>

multidraw::Cut::Cut(char const* _name, TTreeFormula& _formula) :
  name_(_name),
  cut_(_formula)
{
}

multidraw::Cut::Cut(char const* _name, Evaluable::InstanceVal const& _inst, Evaluable::NData const& _ndata/* = nullptr*/) :
  name_(_name),
  cut_(_inst, _ndata)
{
}

multidraw::Cut::~Cut()
{
  for (auto* filler : fillers_)
    delete filler;
}

void
multidraw::Cut::setFormula(TTreeFormula& _formula)
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

void
multidraw::Cut::evaluateAndFill(std::vector<double> const& _eventWeights)
{
  unsigned nD(cut_.getNdata());

  if (!cut_.singleValue())
    instanceMask_->assign(nD, false);

  if (printLevel_ > 2)
    std::cout << "        " << name_ << " has " << nD << " iterations" << std::endl;

  bool any(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (cut_.evalInstance(iD) == 0.)
      continue;

    any = true;

    if (printLevel_ > 2)
        std::cout << "        " << name_ << " iteration " << iD << " pass" << std::endl;

    if (cut_.singleValue())
      (*instanceMask_)[iD] = true;
  }
  
  if (!any)
    return;

  ++counter_;

  for (auto* filler : fillers_)
    filler->fill(_eventWeights, instanceMask_);
}
