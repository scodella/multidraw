#include "../interface/Plot2DFiller.h"
#include "../interface/FormulaLibrary.h"

#include "TDirectory.h"

#include <iostream>
#include <sstream>
#include <thread>

multidraw::Plot2DFiller::Plot2DFiller(TH2& _hist, TTreeFormulaCachedPtr const& _xexpr, TTreeFormulaCachedPtr const& _yexpr, TTreeFormulaCachedPtr const& _reweight/* = nullptr*/) :
  ExprFiller(_reweight),
  hist_(_hist)
{
  exprs_.push_back(_xexpr);
  exprs_.push_back(_yexpr);
}

multidraw::Plot2DFiller::Plot2DFiller(Plot2DFiller const& _orig) :
  ExprFiller(_orig),
  hist_(_orig.hist_)
{
}

multidraw::Plot2DFiller::~Plot2DFiller()
{
  if (isClone_)
    delete &hist_;
}

multidraw::ExprFiller*
multidraw::Plot2DFiller::threadClone(FormulaLibrary& _library) const
{
  std::stringstream name;
  name << hist_.GetName() << "_thread" << std::this_thread::get_id();
  hist_.GetDirectory()->cd();
  auto* hist(static_cast<TH2*>(hist_.Clone(name.str().c_str())));

  auto& xexpr(_library.getFormula(exprs_[0]->GetTitle()));
  auto& yexpr(_library.getFormula(exprs_[1]->GetTitle()));
  TTreeFormulaCachedPtr reweight{};
  if (reweight_)
    reweight = _library.getFormula(reweight_->GetTitle());

  auto* clone(new Plot2DFiller(*hist, xexpr, yexpr, reweight));
  clone->setClone();

  return clone;
}

void
multidraw::Plot2DFiller::threadMerge(ExprFiller& _other)
{
  auto* hist(static_cast<TH2 const*>(&_other.getObj()));
  hist_.Add(hist);
}

void
multidraw::Plot2DFiller::doFill_(unsigned _iD)
{
  if (printLevel_ > 3) {
    std::cout << "            Fill(" << exprs_[0]->EvalInstance(_iD) << ", ";
    std::cout << exprs_[1]->EvalInstance(_iD) << "; " << entryWeight_ << ")" << std::endl;
  }

  double x(exprs_[0]->EvalInstance(_iD));
  double y(exprs_[1]->EvalInstance(_iD));

  hist_.Fill(x, y, entryWeight_);
}
