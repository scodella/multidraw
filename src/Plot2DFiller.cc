#include "../interface/Plot2DFiller.h"
#include "../interface/FormulaLibrary.h"

#include <iostream>
#include <sstream>
#include <thread>

multidraw::Plot2DFiller::Plot2DFiller(TH2& _hist, char const* _xexpr, char const* _yexpr, char const* _reweight/* = ""*/) :
  ExprFiller(_hist, _reweight)
{
  exprs_.push_back(_xexpr);
  exprs_.push_back(_yexpr);
}

multidraw::Plot2DFiller::Plot2DFiller(TObjArray& _histlist, char const* _xexpr, char const* _yexpr, char const* _reweight/* = ""*/) :
  ExprFiller(_histlist, _reweight),
  categorized_(true)
{
  exprs_.push_back(_xexpr);
  exprs_.push_back(_yexpr);
}

multidraw::Plot2DFiller::Plot2DFiller(Plot2DFiller const& _orig) :
  ExprFiller(_orig),
  categorized_(_orig.categorized_)
{
}

multidraw::Plot2DFiller::Plot2DFiller(TH2& _hist, Plot2DFiller const& _orig) :
  ExprFiller(_hist, _orig)
{
}

multidraw::Plot2DFiller::Plot2DFiller(TObjArray& _histlist, Plot2DFiller const& _orig) :
  ExprFiller(_histlist, _orig),
  categorized_(true)
{
}

TH2 const&
multidraw::Plot2DFiller::getHist(int _icat/* = -1*/) const
{
  if (categorized_) {
    auto& array(static_cast<TObjArray&>(tobj_));
    if (_icat >= array.GetEntriesFast())
      throw std::runtime_error(TString::Format("Category index out of bounds: index %d >= maximum %d", _icat, array.GetEntriesFast()).Data());
    return static_cast<TH2&>(*array.UncheckedAt(_icat));
  }
  else
    return static_cast<TH2&>(tobj_);
}

TH2&
multidraw::Plot2DFiller::getHist(int _icat/* = -1*/)
{
  if (categorized_) {
    auto& array(static_cast<TObjArray&>(tobj_));
    if (_icat >= array.GetEntriesFast())
      throw std::runtime_error(TString::Format("Category index out of bounds: index %d >= maximum %d", _icat, array.GetEntriesFast()).Data());
    return static_cast<TH2&>(*array.UncheckedAt(_icat));
  }
  else
    return static_cast<TH2&>(tobj_);
}

void
multidraw::Plot2DFiller::doFill_(unsigned _iD, int _icat/* = -1*/)
{
  if (printLevel_ > 3) {
    std::cout << "            Fill(" << compiledExprs_[0]->EvalInstance(_iD) << ", ";
    std::cout << compiledExprs_[1]->EvalInstance(_iD) << "; " << entryWeight_ << ")" << std::endl;
  }

  double x(compiledExprs_[0]->EvalInstance(_iD));
  double y(compiledExprs_[1]->EvalInstance(_iD));

  auto& hist(getHist(_icat));

  hist.Fill(x, y, entryWeight_);
}

multidraw::ExprFiller*
multidraw::Plot2DFiller::clone_()
{
  if (categorized_) {
    auto& myArray(static_cast<TObjArray&>(tobj_));

    auto* array(new TObjArray());
    array->SetOwner(true);

    for (auto* obj : myArray) {
      std::stringstream name;
      name << obj->GetName() << "_thread" << std::this_thread::get_id();

      array->Add(obj->Clone(name.str().c_str()));
    }

    return new Plot2DFiller(*array, *this);
  }
  else {
    auto& myHist(getHist());

    std::stringstream name;
    name << myHist.GetName() << "_thread" << std::this_thread::get_id();

    auto* hist(static_cast<TH2*>(myHist.Clone(name.str().c_str())));

    return new Plot2DFiller(*hist, *this);
  }
}

void
multidraw::Plot2DFiller::mergeBack_()
{
  if (categorized_) {
    auto& cloneSource(static_cast<Plot2DFiller&>(*cloneSource_));
    auto& myArray(static_cast<TObjArray&>(tobj_));

    for (int icat(0); icat < myArray.GetEntries(); ++icat)
      cloneSource.getHist(icat).Add(&getHist(icat));
  }
  else {
    auto& sourceHist(static_cast<TH2&>(cloneSource_->getObj()));
    sourceHist.Add(&getHist());
  }
}
