#include "../interface/Evaluable.h"

multidraw::Evaluable::Evaluable(InstanceVal const& _inst, NData const& _ndata/* = nullptr*/)
{
  set(_inst, _ndata);
}

multidraw::Evaluable::Evaluable(TTreeFormulaCachedPtr const& _formula)
{
  set(_formula);
}

multidraw::Evaluable::Evaluable(Evaluable const& _orig) :
  instanceVal_(_orig.instanceVal_),
  ndata_(_orig.ndata_),
  formula_(_orig.formula_),
  singleValue_(_orig.singleValue_)
{
}

multidraw::Evaluable&
multidraw::Evaluable::operator=(Evaluable const& _rhs)
{
  instanceVal_ = _rhs.instanceVal_;
  ndata_ = _rhs.ndata_;
  formula_ = _rhs.formula_;
  singleValue_ = _rhs.singleValue_;
  return *this;
}

void
multidraw::Evaluable::set(InstanceVal const& _inst, NData const& _ndata/* = nullptr*/)
{
  formula_.reset();

  instanceVal_ = _inst;
  if (_ndata) {
    singleValue_ = false;
    ndata_ = _ndata;
  }
  else {
    singleValue_ = true;
    ndata_ = []()->unsigned { return 1; };
  }
}

void
multidraw::Evaluable::set(TTreeFormulaCachedPtr const& _formula)
{
  formula_ = _formula;
  singleValue_ = false;
}

void
multidraw::Evaluable::reset()
{
  instanceVal_ = nullptr;
  ndata_ = nullptr;
  formula_.reset();
  singleValue_ = false;
}

bool
multidraw::Evaluable::singleValue() const
{
  if (formula_)
    return formula_->GetMultiplicity() == 0;
  else
    return singleValue_;
}

unsigned
multidraw::Evaluable::getNdata()
{
  if (formula_)
    return formula_->GetNdata();
  else
    return ndata_();
}

double
multidraw::Evaluable::evalInstance(unsigned _i)
{
  if (formula_)
    return formula_->EvalInstance(_i);
  else
    return instanceVal_(_i);
}
