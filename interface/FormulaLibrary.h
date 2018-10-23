#ifndef multidraw_FormulaLibrary_h
#define multidraw_FormulaLibrary_h

#include "TTreeFormulaCached.h"

#include "TString.h"

#include <map>
#include <memory>

class TTree;

namespace multidraw {

  class FormulaLibrary : public std::map<TString, std::shared_ptr<TTreeFormulaCached>> {
  public:
    FormulaLibrary(TTree&);
    ~FormulaLibrary() {}

    //! Find the formula object matching the expr. If not found, create new.
    std::shared_ptr<TTreeFormulaCached> const& getFormula(char const* expr);

    //! Erase formulas with only one ref count (i.e. by myself)
    void prune();

  private:
    TTree& tree_;
  };

}

#endif
