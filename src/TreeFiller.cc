#include "../interface/TreeFiller.h"
#include "../interface/TTreeFormulaCached.h"

#include <iostream>

multidraw::TreeFiller::TreeFiller(TTree& _tree, FormulaLibrary& _library, std::shared_ptr<TTreeFormulaCached> const& _reweight/* = nullptr*/) :
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
  bvalues_(_orig.bvalues_)
{
  tree_.SetBranchAddress("weight", &entryWeight_);

  bvalues_.reserve(NBRANCHMAX);

  // rely on the fact that the branch order should be aligned
  auto* branches(tree_.GetListOfBranches());
  for (int iB(0); iB != branches->GetEntries(); ++iB)
    tree_.SetBranchAddress(branches->At(iB)->GetName(), &bvalues_[iB]);
}

void
multidraw::TreeFiller::addBranch(char const* _bname, char const* _expr)
{
  if (bvalues_.size() == NBRANCHMAX)
    throw std::runtime_error("Cannot add any more branches");

  bvalues_.resize(bvalues_.size() + 1);
  tree_.Branch(_bname, &bvalues_.back(), TString::Format("%s/D", _bname));

  exprs_.push_back(library_.getFormula(_expr));
}

void
multidraw::TreeFiller::doFill_(unsigned _iD)
{
  if (printLevel_ > 3)
    std::cout << "            Fill(";

  for (unsigned iE(0); iE != exprs_.size(); ++iE) {
    if (printLevel_ > 3) {
      std::cout << exprs_[iE]->EvalInstance(_iD);
      if (iE != exprs_.size() - 1)
        std::cout << ", ";
    }

    bvalues_[iE] = exprs_[iE]->EvalInstance(_iD);
  }

  if (printLevel_ > 3)
    std::cout << "; " << entryWeight_ << ")" << std::endl;

  tree_.Fill();
}
