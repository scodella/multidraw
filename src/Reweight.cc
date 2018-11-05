#include "../interface/Reweight.h"

#include "TSpline.h"
#include "TClass.h"

multidraw::Reweight::Reweight(TTreeFormulaCachedPtr const& _formula)
{
  set(_formula);
}
  
multidraw::Reweight::Reweight(TObject const& _source, TTreeFormulaCachedPtr const& _x, TTreeFormulaCachedPtr const& _y/* = nullptr*/, TTreeFormulaCachedPtr const& _z/* = nullptr*/)
{
  set(_source, _x, _y, _z);
}

multidraw::Reweight::Reweight(Reweight const& _orig) :
  formulas_(_orig.formulas_),
  source_(_orig.source_)
{
  if (formulas_.size() == 0)
    return;
  else if (source_ == nullptr)
    setRaw_();
  else if (source_->InheritsFrom(TH1::Class()))
    setTH1_();
  else if (source_->InheritsFrom(TGraph::Class()))
    setTGraph_();
  else if (source_->InheritsFrom(TF1::Class()))
    setTF1_();
}

multidraw::Reweight::~Reweight()
{
  delete spline_;
}

multidraw::Reweight&
multidraw::Reweight::operator=(Reweight const& _rhs)
{
  formulas_ = _rhs.formulas_;
  source_ = _rhs.source_;

  if (formulas_.size() == 0)
    return *this;
  else if (source_ == nullptr)
    setRaw_();
  else if (source_->InheritsFrom(TH1::Class()))
    setTH1_();
  else if (source_->InheritsFrom(TGraph::Class()))
    setTGraph_();
  else if (source_->InheritsFrom(TF1::Class()))
    setTF1_();

  return *this;
}

void
multidraw::Reweight::set(TTreeFormulaCachedPtr const& _formula)
{
  formulas_.assign(1, _formula);
  source_ = nullptr;
  setRaw_();
}

void
multidraw::Reweight::set(TObject const& _source, TTreeFormulaCachedPtr const& _x, TTreeFormulaCachedPtr const& _y/* = nullptr*/, TTreeFormulaCachedPtr const& _z/* = nullptr*/)
{
  formulas_.assign(1, _x);
  if (_y)
    formulas_.push_back(_y);
  if (_z)
    formulas_.push_back(_z);

  source_ = &_source;
  if (source_->InheritsFrom(TH1::Class()))
    setTH1_();
  else if (source_->InheritsFrom(TGraph::Class()))
    setTGraph_();
  else if (source_->InheritsFrom(TF1::Class()))
    setTF1_();
  else
    throw std::runtime_error(TString::Format("Object of incompatible class %s passed to Reweight", source_->IsA()->GetName()).Data());
}

void
multidraw::Reweight::reset()
{
  formulas_.clear();
  source_ = nullptr;
  delete spline_;
  spline_ = nullptr;
  evaluate_ = nullptr;
}

unsigned
multidraw::Reweight::getNdata()
{
  if (formulas_.size() == 0)
    return 1;
  else {
    // use the first dimension
    return formulas_[0]->GetNdata();
  }
}

void
multidraw::Reweight::setRaw_()
{
  evaluate_ = [this](unsigned i)->double { return this->evaluateRaw_(i); };
}

void

multidraw::Reweight::setTH1_()
{
  auto& hist(static_cast<TH1 const&>(*source_));

  if (hist.GetDimension() != int(formulas_.size()))
    throw std::runtime_error(std::string("Invalid number of formulas given for histogram source of type ") + hist.IsA()->GetName());

  evaluate_ = [this](unsigned i)->double { return this->evaluateTH1_(i); };
}

void
multidraw::Reweight::setTGraph_()
{
  delete spline_;
  spline_ = new TSpline3("interpolation", static_cast<TGraph const*>(source_));
  
  evaluate_ = [this](unsigned i)->double { return this->evaluateTGraph_(i); };
}

void
multidraw::Reweight::setTF1_()
{
  auto& fct(static_cast<TF1 const&>(*source_));

  if (fct.GetNdim() != int(formulas_.size()))
    throw std::runtime_error(std::string("Invalid number of formulas given for function source of type ") + fct.IsA()->GetName());

  evaluate_ = [this](unsigned i)->double { return this->evaluateTF1_(i); };
}

double
multidraw::Reweight::evaluateRaw_(unsigned _iD)
{
  formulas_[0]->GetNdata();
  if (_iD != 0)
    formulas_[0]->EvalInstance(0);
  
  return formulas_[0]->EvalInstance(_iD);
}

double
multidraw::Reweight::evaluateTH1_(unsigned _iD)
{
  auto& hist(static_cast<TH1 const&>(*source_));

  double x[3]{};
  for (unsigned iDim(0); iDim != formulas_.size(); ++iDim) {
    auto& formula(formulas_[iDim]);
    formula->GetNdata();
    if (_iD != 0)
      formula->EvalInstance(0);
    
    x[iDim] = formula->EvalInstance(_iD);
  }

  int iBin(hist.FindFixBin(x[0], x[1], x[2]));

  // not handling over/underflow at the moment

  return hist.GetBinContent(iBin);
}

double
multidraw::Reweight::evaluateTGraph_(unsigned _iD)
{
  auto& graph(static_cast<TGraph const&>(*source_));

  formulas_[0]->GetNdata();
  if (_iD != 0)
    formulas_[0]->EvalInstance(0);
  
  double x(formulas_[0]->EvalInstance(_iD));

  return graph.Eval(x, spline_);
}

double
multidraw::Reweight::evaluateTF1_(unsigned _iD)
{
  auto& fct(static_cast<TF1 const&>(*source_));

  double x[3]{};
  for (unsigned iDim(0); iDim != formulas_.size(); ++iDim) {
    auto& formula(formulas_[iDim]);
    formula->GetNdata();
    if (_iD != 0)
      formula->EvalInstance(0);

    x[iDim] = formula->EvalInstance(_iD);
  }
  
  return fct.Eval(x[0], x[1], x[2]);
}
