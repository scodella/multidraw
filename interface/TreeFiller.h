#ifndef multidraw_TreeFiller_h
#define multidraw_TreeFiller_h

#include "TTreeFormulaCached.h"
#include "ExprFiller.h"
#include "FormulaLibrary.h"

#include "TTree.h"

namespace multidraw {

  //! A wrapper class for TTree
  /*!
   * The class is to be used within MultiDraw, and is instantiated by addTree().
   * Arguments:
   *  tree      The actual tree object (the user is responsible to create it)
   *  library   FormulaLibrary to draw formula objects from
   *  reweight  If provided, evalutaed and used as weight for filling the histogram
   */
  class TreeFiller : public ExprFiller {
  public:
    TreeFiller(TTree& tree, FormulaLibrary& library, TTreeFormulaCachedPtr const& reweight = nullptr);
    TreeFiller(TreeFiller const&);
    ~TreeFiller() {}

    void addBranch(char const* bname, char const* expr);

    TObject const& getObj() const override { return tree_; }
    TTree const& getTree() const { return tree_; }

    static unsigned const NBRANCHMAX = 128;

  private:
    void doFill_(unsigned) override;

    TTree& tree_;
    FormulaLibrary& library_;
    std::vector<double> bvalues_{};
  };

}

#endif
