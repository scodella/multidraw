#ifndef multidraw_FormulaLibrary_h
#define multidraw_FormulaLibrary_h

#include "TTreeFormulaCached.h"

#include "TString.h"

#include <map>
#include <list>
#include <memory>

class TTree;

namespace multidraw {

  class FormulaLibrary {
  public:
    FormulaLibrary(TTree&);
    ~FormulaLibrary() {}

    //! Create a new formula object with a shared cache.
    /*!
     * ExprFiller uses a TTreeFormulaManager to aggregate formulas. Adding a formula to a manager overwrites
     * an attribute of the formula object itself. Since the grouping can be different for each filler, this
     * means we cannot cache the formulas themselves and reuse among the fillers.
     */
    TTreeFormulaCachedPtr getFormula(char const* expr, bool silent = false);

    void updateFormulaLeaves();
    void resetCache();

    unsigned size() const { return formulas_.size(); }
    void prune();

  private:
    TTree& tree_;

    std::map<TString, TTreeFormulaCached::CachePtr> caches_{};
    std::list<std::weak_ptr<TTreeFormulaCached>> formulas_{};
  };

}

#endif
