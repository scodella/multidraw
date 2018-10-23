#include "../interface/Plot2DFiller.h"

#include <iostream>

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
