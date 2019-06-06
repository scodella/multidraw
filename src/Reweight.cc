#include "../interface/Reweight.h"
#include "../interface/FormulaLibrary.h"

#include "TSpline.h"
#include "TClass.h"
#include "TTreeFormulaManager.h"

multidraw::Reweight::Reweight(TTreeFormulaCachedPtr const& _xformula, TTreeFormulaCachedPtr const& _yformula/* = nullptr*/)
{
  set(_xformula, _yformula);
}
  
multidraw::Reweight::Reweight(TObject const& _source, TTreeFormulaCachedPtr const& _x, TTreeFormulaCachedPtr const& _y/* = nullptr*/, TTreeFormulaCachedPtr const& _z/* = nullptr*/)
{
  set(_source, _x, _y, _z);
}

multidraw::Reweight::~Reweight()
{
  reset();
}

void
multidraw::Reweight::set(TTreeFormulaCachedPtr const& _xformula, TTreeFormulaCachedPtr const& _yformula/* = nullptr*/)
{
  formulas_.assign(1, _xformula);
  if (_yformula)
    formulas_.push_back(_yformula);

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

  if (formulas_.size() != 1) {
    auto* manager(formulas_[0]->GetManager());
    for (unsigned iF(1); iF != formulas_.size(); ++iF)
      manager->Add(formulas_[iF].get());

    manager->Sync();
  }

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
  if (formulas_.empty())
    return 1;

  return formulas_[0]->GetManager()->GetNdata();
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


multidraw::FactorizedReweight::FactorizedReweight(Reweight* _ptr1, Reweight* _ptr2)
{
  set(_ptr1, _ptr2);
}

void
multidraw::FactorizedReweight::set(Reweight* _ptr1, Reweight* _ptr2)
{
  subReweights_[0] = _ptr1;
  subReweights_[1] = _ptr2;
}

void
multidraw::FactorizedReweight::reset()
{
  delete subReweights_[0];
  delete subReweights_[1];
  subReweights_[0] = nullptr;
  subReweights_[1] = nullptr;

  Reweight::reset();
}

TTreeFormulaCached*
multidraw::FactorizedReweight::getFormula(unsigned i/* = 0*/) const
{
  if (i < subReweights_[0]->getNdim())
    return subReweights_[0]->getFormula(i);
  else
    return subReweights_[1]->getFormula(i - subReweights_[0]->getNdim());
}

multidraw::ReweightSource::ReweightSource(ReweightSource const& _orig) :
  xexpr_(_orig.xexpr_),
  yexpr_(_orig.yexpr_),
  source_(_orig.source_)
{
  if (_orig.subReweights_[0] != nullptr) {
    subReweights_[0] = new ReweightSource(*_orig.subReweights_[0]);
    subReweights_[1] = new ReweightSource(*_orig.subReweights_[1]);
  }
}

multidraw::ReweightPtr
multidraw::ReweightSource::compile(FormulaLibrary& _library) const
{
  if (subReweights_[0] != nullptr)
    return std::unique_ptr<Reweight>(new FactorizedReweight(subReweights_[0]->compile(_library).release(), subReweights_[1]->compile(_library).release()));

  auto xformula(_library.getFormula(xexpr_));
  auto yformula(_library.getFormula(yexpr_));

  if (source_ == nullptr)
    return std::make_unique<Reweight>(xformula, yformula);
  else
    return std::make_unique<Reweight>(*source_, xformula, yformula);
}
