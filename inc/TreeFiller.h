#ifndef multidraw_TreeFiller_h
#define multidraw_TreeFiller_h

#include "ExprFiller.h"

#include "TTree.h"

namespace multidraw {

  //! A wrapper class for TTree
  /*!
   * The class is to be used within MultiDraw, and is instantiated by addTree().
   * Arguments:
   *  TTree& tree             The actual tree object (the user is responsible to create it)
   *  TTreeFormula* cuts      If provided and evaluates to 0, the tree is not filled
   *  TTreeFormula* reweight  If provided, evalutaed and used as weight for filling the histogram
   */
  class TreeFiller : public ExprFiller {
  public:
    TreeFiller() {}
    TreeFiller(TTree& tree, TTreeFormula* cuts = nullptr, TTreeFormula* reweight = nullptr);
    TreeFiller(TreeFiller const&);
    ~TreeFiller() {}

    void addBranch(char const* bname, TTreeFormula& expr);

    TObject const* getObj() const override { return tree_; }
    TTree const* getTree() const { return tree_; }

    static unsigned const NBRANCHMAX = 128;

  private:
    void doFill_(unsigned) override;

    TTree* tree_{nullptr};
    std::vector<double> bvalues_{};
  };

}

#endif
