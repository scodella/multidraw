#ifndef multidraw_Cut_h
#define multidraw_Cut_h

#include "TTreeFormulaCached.h"
#include "FormulaLibrary.h"

#include "TString.h"

#include <vector>

namespace multidraw {

  class ExprFiller;

  class Cut {
  public:
    Cut(char const* name, char const* expr = "");
    ~Cut();

    TString getName() const;
    void setPrintLevel(int);

    unsigned getNFillers() const { return fillers_.size(); }
    ExprFiller const* getFiller(unsigned i) const { return fillers_.at(i); }

    void setCutExpr(char const* expr) { cutExpr_ = expr; }
    TString const& getCutExpr() const { return cutExpr_; }

    void addCategory(char const* expr) { categoryExprs_.emplace_back(expr); }
    void setCategorization(char const* expr);
    int getNCategories() const;

    void addFiller(ExprFiller& _filler) { fillers_.push_back(&_filler); }

    void bindTree(FormulaLibrary&);
    void unlinkTree();
    Cut* threadClone(FormulaLibrary&) const;

    bool cutDependsOn(TTree const*) const;

    void initialize();
    bool evaluate();
    void fillExprs(std::vector<double> const& eventWeights);

    unsigned getCount() const { return counter_; }

  protected:
    TString name_{""};
    TString cutExpr_{""};
    std::vector<TString> categoryExprs_{};
    TString categorizationExpr_{""};
    std::vector<ExprFiller*> fillers_{};
    int printLevel_{0};
    unsigned counter_{0};

    std::vector<int> categoryIndex_;
    TTreeFormulaCachedPtr compiledCut_{nullptr};
    std::vector<TTreeFormulaCachedPtr> compiledCategories_{};
    TTreeFormulaCachedPtr compiledCategorization_{};
  };

}

#endif
