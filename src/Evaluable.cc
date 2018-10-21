#include "../inc/Evaluable.h"

multidraw::Evaluable::Evaluable(InstanceVal const& _inst, NData const& _ndata/* = nullptr*/)
{
  set(_inst, _ndata);
}

multidraw::Evaluable::Evaluable(TTreeFormula& _formula)
{
  set(_formula);
}

multidraw::Evaluable::Evaluable(Evaluable const& _orig) :
  instanceVal_(_orig.instanceVal_),
  ndata_(_orig.ndata_),
  singleValue_(_orig.singleValue_)
{
}

multidraw::Evaluable&
multidraw::Evaluable::operator=(Evaluable const& _rhs)
{
  instanceVal_ = _rhs.instanceVal_;
  ndata_ = _rhs.ndata_;
  singleValue_ = _rhs.singleValue_;
  return *this;
}

void
multidraw::Evaluable::set(InstanceVal const& _inst, NData const& _ndata/* = nullptr*/)
{
  instanceVal_ = _inst;
  if (_ndata) {
    singleValue_ = false;
    ndata_ = _ndata;
  }
  else {
    singleValue_ = true;
    ndata_ = []()->UInt_t { return 1; };
  }
}

void
multidraw::Evaluable::set(TTreeFormula& _formula)
{
  instanceVal_ = [&_formula](UInt_t i)->Double_t { return _formula.EvalInstance(i); };
  // We need to call GetNdata() even if GetMultiplicity() is 0 - cannot set ndata_ to { return 1.; }
  ndata_ = [&_formula]()->UInt_t { return _formula.GetNdata(); };
  if (_formula.GetMultiplicity() != 0)
    singleValue_ = false;
  else
    singleValue_ = true;
}
