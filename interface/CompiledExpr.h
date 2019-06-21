#ifndef multidraw_CompiledExpr_h
#define multidraw_CompiledExpr_h

#include "TTreeFormulaCached.h"
#include "TTreeFunction.h"

#include <memory>

namespace multidraw {

  class CompiledExprSource {
  public:
    CompiledExprSource(char const* formula) : formula_(formula) {}
    CompiledExprSource(std::shared_ptr<TTreeFunction> const& function) : function_(function) {}
    CompiledExprSource(CompiledExprSource const& orig) : formula_(orig.formula_), function_(orig.function_) {}

    char const* getFormula() const { return formula_; }
    TTreeFunction const* getFunction() const { return function_.get(); }

  private:
    TString formula_{};
    std::shared_ptr<TTreeFunction> function_{};
  };

  class CompiledExpr {
  public:
    CompiledExpr(TTreeFormulaCached& formula) : formula_(&formula) {}
    CompiledExpr(TTreeFunction&);
    ~CompiledExpr() {}

    TTreeFormulaCached* getFormula() const { return formula_; }
    TTreeFunction* getFunction() const { return function_; }

    unsigned getNdata();
    double evaluate(unsigned);

  private:
    TTreeFormulaCached* formula_{};
    TTreeFunction* function_{};
  };

  typedef std::unique_ptr<CompiledExpr> CompiledExprPtr;

}

#endif
