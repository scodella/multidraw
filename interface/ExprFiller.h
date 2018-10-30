#ifndef multidraw_ExprFiller_h
#define multidraw_ExprFiller_h

#include "TTreeFormulaCached.h"

#include "TString.h"

#include <vector>

namespace multidraw {

  class FormulaLibrary;

  //! Filler object base class with expressions, a cut, and a reweight.
  /*!
   * Inherited by Plot (histogram) and Tree (tree). Does not own any of
   * the TTreeFormula objects by default.
   */
  class ExprFiller {
  public:
    ExprFiller(TTreeFormulaCachedPtr const& reweight = nullptr);
    ExprFiller(ExprFiller const&);
    virtual ~ExprFiller() {}

    void fill(std::vector<double> const& eventWeights, std::vector<bool> const* = nullptr);

    virtual TObject const& getObj() const = 0;
    virtual TObject& getObj() = 0;

    void resetCount() { counter_ = 0; }
    unsigned getCount() const { return counter_; }

    void setPrintLevel(int l) { printLevel_ = l; }

    virtual ExprFiller* threadClone(FormulaLibrary&) const = 0;
    virtual void threadMerge(TObject&) = 0;

  protected:
    virtual void doFill_(unsigned) = 0;
    void setClone() { isClone_ = true; }

    std::vector<TTreeFormulaCachedPtr> exprs_{};
    TTreeFormulaCachedPtr reweight_{nullptr};
    double entryWeight_{1.};
    unsigned counter_{0};

    bool isClone_{false};

    int printLevel_{0};
  };

}

#endif
