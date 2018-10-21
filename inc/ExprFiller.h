#ifndef multidraw_ExprFiller_h
#define multidraw_ExprFiller_h

#include "TString.h"

#include <vector>

class TObject;
class TTreeFormula;

namespace multidraw {

  //! Filler object base class with expressions, a cut, and a reweight.
  /*!
   * Inherited by Plot (histogram) and Tree (tree). Does not own any of
   * the TTreeFormula objects by default.
   */
  class ExprFiller {
  public:
    ExprFiller(TTreeFormula* reweight = nullptr);
    ExprFiller(ExprFiller const&);
    virtual ~ExprFiller() {}

    unsigned getNdim() const { return exprs_.size(); }
    TTreeFormula const* getExpr(unsigned iE = 0) const { return exprs_.at(iE); }
    TTreeFormula* getExpr(unsigned iE = 0) { return exprs_.at(iE); }
    TTreeFormula const* getReweight() const { return reweight_; }

    void fill(std::vector<double> const& eventWeights, std::vector<bool> const* = nullptr);

    virtual TObject const& getObj() const = 0;

    void resetCount() { counter_ = 0; }
    unsigned getCount() const { return counter_; }

    void setPrintLevel(int l) { printLevel_ = l; }

  protected:
    virtual void doFill_(unsigned) = 0;

    std::vector<TTreeFormula*> exprs_{};
    TTreeFormula* reweight_{nullptr};
    double entryWeight_{1.};
    unsigned counter_{0};

    int printLevel_{0};
  };

}

#endif
