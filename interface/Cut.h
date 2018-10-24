#ifndef multidraw_Cut_h
#define multidraw_Cut_h

#include "Evaluable.h"
#include "TTreeFormulaCached.h"

#include "TString.h"

#include <vector>

namespace multidraw {

  class ExprFiller;

  class Cut {
  public:
    Cut(char const* name, TTreeFormulaCachedPtr const&);
    Cut(char const* name, Evaluable::InstanceVal const&, Evaluable::NData const& = nullptr);
    ~Cut();

    TString getName() const;
    unsigned getNFillers() const { return fillers_.size(); }
    ExprFiller const* getFiller(unsigned i) { return fillers_.at(i); }

    void addFiller(ExprFiller& _filler) { fillers_.push_back(&_filler); }
    void setFormula(TTreeFormulaCachedPtr const&);
    void setFormula(Evaluable::InstanceVal const&, Evaluable::NData const& = nullptr);
    void setPrintLevel(int);
    void resetCount();
    unsigned getCount() const { return counter_; }
    bool evaluate();
    void fillExprs(std::vector<double> const& eventWeights);

  protected:
    TString name_;
    Evaluable cut_;
    std::vector<ExprFiller*> fillers_;
    int printLevel_{0};
    unsigned counter_{0};

    std::vector<bool>* instanceMask_{nullptr};
  };

}

#endif
