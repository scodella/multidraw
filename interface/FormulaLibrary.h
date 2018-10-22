#ifndef multidraw_FormulaLibrary_h
#define multidraw_FormulaLibrary_h

#include "TTreeFormulaCached.h"

#include "TString.h"

#include <map>

class TTree;

namespace multidraw {

  class FormulaLibrary : public std::map<TString, TTreeFormulaCached*> {
  public:
    FormulaLibrary(TTree&);
    ~FormulaLibrary();

    //! Find the formula object matching the expr. If not found, create new.
    TTreeFormulaCached* getFormula(char const* expr);
    //! Delete the formula object and set the pointer to null.
    void deleteFormula(TTreeFormulaCached*&);

  private:
    TTree& tree_;
  };

}

#endif
