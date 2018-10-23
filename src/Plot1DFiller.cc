#include "../interface/Plot1DFiller.h"
#include "../interface/TTreeFormulaCached.h"

#include <iostream>

multidraw::Plot1DFiller::Plot1DFiller(TH1& _hist, std::shared_ptr<TTreeFormulaCached> const& _expr, std::shared_ptr<TTreeFormulaCached> const& _reweight/* = nullptr*/, Plot1DFiller::OverflowMode _mode/* = kDefault*/) :
  ExprFiller(_reweight),
  hist_(_hist),
  overflowMode_(_mode)
{
  exprs_.push_back(_expr);
}

multidraw::Plot1DFiller::Plot1DFiller(Plot1DFiller const& _orig) :
  ExprFiller(_orig),
  hist_(_orig.hist_),
  overflowMode_(_orig.overflowMode_)
{
}

void
multidraw::Plot1DFiller::doFill_(unsigned _iD)
{
  if (printLevel_ > 3)
    std::cout << "            Fill(" << exprs_[0]->EvalInstance(_iD) << "; " << entryWeight_ << ")" << std::endl;

  double x(exprs_[0]->EvalInstance(_iD));

  switch (overflowMode_) {
  case OverflowMode::kDefault:
    break;
  case OverflowMode::kDedicated:
    if (x > hist_.GetXaxis()->GetBinLowEdge(hist_.GetNbinsX()))
      x = hist_.GetXaxis()->GetBinLowEdge(hist_.GetNbinsX());
    break;
  case OverflowMode::kMergeLast:
    if (x > hist_.GetXaxis()->GetBinUpEdge(hist_.GetNbinsX()))
      x = hist_.GetXaxis()->GetBinLowEdge(hist_.GetNbinsX());
    break;
  }

  hist_.Fill(x, entryWeight_);
}
