#ifndef multidraw_Cut_h
#define multidraw_Cut_h

#include "TString.h"

#include <functional>
#include <vector>

class TTreeFormula;

namespace multidraw {

  class ExprFiller;

  class Cut {
  public:
    typedef std::function<bool(void)> CutFunc;

    Cut(char const* name, TTreeFormula&);
    Cut(char const* name, CutFunc const&);
    ~Cut();

    void addFiller(ExprFiller& _filler) { fillers_.push_back(&_filler); }
    void setFormula(TTreeFormula&);

  protected:
    TString name_;
    TTreeFormula* formula_{nullptr};
    CutFunc func_{nullptr};
    std::vector<ExprFiller*> fillers_;
  };

}

#endif
