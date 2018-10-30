#include "../interface/Plot1DFiller.h"
#include "../interface/FormulaLibrary.h"

#include "TDirectory.h"

#include <iostream>
#include <sstream>
#include <thread>

multidraw::Plot1DFiller::Plot1DFiller(TH1& _hist, TTreeFormulaCachedPtr const& _expr, TTreeFormulaCachedPtr const& _reweight/* = nullptr*/, Plot1DFiller::OverflowMode _mode/* = kDefault*/) :
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

multidraw::Plot1DFiller::~Plot1DFiller()
{
  if (isClone_)
    delete &hist_;
}

multidraw::ExprFiller*
multidraw::Plot1DFiller::threadClone(FormulaLibrary& _library) const
{
  std::stringstream name;
  name << hist_.GetName() << "_thread" << std::this_thread::get_id();
  hist_.GetDirectory()->cd();
  auto* hist(static_cast<TH1*>(hist_.Clone(name.str().c_str())));

  auto& expr(_library.getFormula(exprs_[0]->GetTitle()));
  TTreeFormulaCachedPtr reweight{};
  if (reweight_)
    reweight = _library.getFormula(reweight_->GetTitle());

  auto* clone(new Plot1DFiller(*hist, expr, reweight, overflowMode_));
  clone->setClone();

  return clone;
}

void
multidraw::Plot1DFiller::threadMerge(TObject& _main)
{
  TH1& mainHist(static_cast<TH1&>(_main));
  mainHist.Add(&hist_);
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
