#include "../interface/TreeFiller.h"
#include "../interface/FormulaLibrary.h"

#include "TDirectory.h"

#include <iostream>
#include <sstream>
#include <thread>

multidraw::TreeFiller::TreeFiller(TTree& _tree, FormulaLibrary& _library, TTreeFormulaCachedPtr const& _reweight/* = nullptr*/) :
  ExprFiller(_reweight),
  tree_(_tree),
  library_(_library)
{
  tree_.Branch("weight", &entryWeight_, "weight/D");

  bvalues_.reserve(NBRANCHMAX);
}

multidraw::TreeFiller::TreeFiller(TreeFiller const& _orig) :
  ExprFiller(_orig),
  tree_(_orig.tree_),
  library_(_orig.library_),
  bnames_(_orig.bnames_)
{
  tree_.SetBranchAddress("weight", &entryWeight_);

  bvalues_.reserve(NBRANCHMAX);
  bvalues_ = _orig.bvalues_;

  for (unsigned iB(0); iB != bvalues_.size(); ++iB)
    tree_.SetBranchAddress(bnames_[iB], &bvalues_[iB]);
}

multidraw::TreeFiller::~TreeFiller()
{
  if (isClone_) {
    tree_.ResetBranchAddresses();
    auto* dir(tree_.GetDirectory());
    delete dir;
  }
}

void
multidraw::TreeFiller::addBranch(char const* _bname, char const* _expr)
{
  if (bvalues_.size() == NBRANCHMAX)
    throw std::runtime_error("Cannot add any more branches");

  bvalues_.resize(bvalues_.size() + 1);
  tree_.Branch(_bname, &bvalues_.back(), TString::Format("%s/D", _bname));

  bnames_.emplace_back(_bname);
  exprs_.push_back(library_.getFormula(_expr));
}

multidraw::ExprFiller*
multidraw::TreeFiller::threadClone(FormulaLibrary& _library) const
{
  std::stringstream name;
  name << "tree_thread" << std::this_thread::get_id();
  tree_.GetDirectory()->mkdir(name.str().c_str())->cd();
  
  auto* tree(new TTree(tree_.GetName(), tree_.GetTitle()));

  TTreeFormulaCachedPtr reweight{};
  if (reweight_)
    reweight = _library.getFormula(reweight_->GetTitle());

  auto* clone(new TreeFiller(*tree, _library, reweight));
  clone->setClone();

  for (unsigned iE(0); iE != exprs_.size(); ++iE)
    clone->addBranch(bnames_[iE], exprs_[iE]->GetTitle());

  return clone;
}

void
multidraw::TreeFiller::threadMerge(ExprFiller& _other)
{
  auto* tree(static_cast<TTree*>(&_other.getObj()));
  TObjArray arr;
  arr.Add(tree);
  tree_.Merge(&arr);
}

void
multidraw::TreeFiller::doFill_(unsigned _iD)
{
  if (printLevel_ > 3)
    std::cout << "            Fill(";

  for (unsigned iE(0); iE != exprs_.size(); ++iE) {
    bvalues_[iE] = exprs_[iE]->EvalInstance(_iD);

    if (printLevel_ > 3) {
      std::cout << bvalues_[iE];
      if (iE != exprs_.size() - 1)
        std::cout << ", ";
    }
  }

  if (printLevel_ > 3)
    std::cout << "; " << entryWeight_ << ")" << std::endl;

  tree_.Fill();
}
