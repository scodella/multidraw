#ifndef multidraw_TTreeFormulaCached_h
#define multidraw_TTreeFormulaCached_h

#include "TTreeFormula.h"

#include <vector>
#include <utility>

class TObjArray;

//! Cached version of TTreeFormula.
/*!
 * User of this class must first call ResetCache() and GetNdata() in the order to properly
 * access the instance value. Value of fNdataCache is returned for the second and subsequent
 * calls to GetNdata(). Instances are evaluated and cached at the first call of EvalInstance()
 * for the respective indices.
 *
 * Note: override keyword in this class definition is commented out to avoid getting compiler
 * warnings (-Winconsistent-missing-override).
 */
class TTreeFormulaCached : public TTreeFormula {
public:
  TTreeFormulaCached(char const* name, char const* formula, TTree* tree) : TTreeFormula(name, formula, tree) {}
  ~TTreeFormulaCached() {}

  Int_t GetNdata()/* override*/;
  Double_t EvalInstance(Int_t, char const* [] = nullptr)/* override*/;

  void ResetCache() { fNdataCache = -1; }

private:
  Int_t fNdataCache{-1};
  std::vector<std::pair<Bool_t, Double_t>> fCache{};

  ClassDef(TTreeFormulaCached, 1)
};

/*
  TFormula has no foolproof mechanism to signal a failure of expression compilation.
  For normal expressions like TTreeFormula f("formula", "bogus", tree), we get f.GetTree() == 0.
  However if a TTreePlayer function (Sum$, Max$, etc.) is used, the top-level expression
  is considered valid even if the enclosed expression is not, and GetTree() returns the tree
  address.
  The only way we catch all compilation failures is to use the error message using ROOT
  error handling mechanism.
  Functions NewTTreeFormula and NewTTreeFormulaCached are written for this purpose. They are
  just `new TTreeFormula(Cached)` calls, wrapped in error handling routines.
*/

//! Create a TTreeFormula with error handling
TTreeFormula* NewTTreeFormula(char const* name, char const* expr, TTree*);

//! Create a TTreeFormulaCached with error handling
TTreeFormulaCached* NewTTreeFormulaCached(char const* name, char const* expr, TTree*);

#endif
