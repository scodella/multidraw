#ifndef multidraw_Reweight_h
#define multidraw_Reweight_h

#include "TTreeFormulaCached.h"

#include "TH1.h"
#include "TGraph.h"
#include "TF1.h"

#include <functional>

class TSpline3;

namespace multidraw {

  class Reweight {
  public:
    Reweight() {}
    Reweight(TTreeFormulaCachedPtr const&, TTreeFormulaCachedPtr const& = nullptr);
    Reweight(TObject const&, TTreeFormulaCachedPtr const&, TTreeFormulaCachedPtr const& = nullptr, TTreeFormulaCachedPtr const& = nullptr);
    ~Reweight();

    //! Set the tree formula to reweight with.
    void set(TTreeFormulaCachedPtr const& xformula, TTreeFormulaCachedPtr const& yformula = nullptr);
    //! Reweight through a histogram, a graph, or a function. Formulas return the x, y, z values to be fed to the object.
    void set(TObject const&, TTreeFormulaCachedPtr const&, TTreeFormulaCachedPtr const& = nullptr, TTreeFormulaCachedPtr const& = nullptr);

    void reset();

    unsigned getNdim() const { return formulas_.size(); }
    TTreeFormulaCached* getFormula(unsigned i = 0) const { return formulas_.at(i).get(); }
    TObject const* getSource() const { return source_; }

    unsigned getNdata();
    double evaluate(unsigned i = 0) const { if (evaluate_) return evaluate_(i); else return 1.; }

  private:
    void setRaw_();
    void setTH1_();
    void setTGraph_();
    void setTF1_();

    double evaluateRaw_(unsigned);
    double evaluateTH1_(unsigned);
    double evaluateTGraph_(unsigned);
    double evaluateTF1_(unsigned);

    //! One entry per source dimension
    std::vector<TTreeFormulaCachedPtr> formulas_{};
    TObject const* source_{nullptr};
    TSpline3* spline_{nullptr};

    std::function<double(unsigned)> evaluate_{};
  };

  typedef std::unique_ptr<Reweight> ReweightPtr;

  class FormulaLibrary;

  class ReweightSource {
  public:
    ReweightSource() {}
    ReweightSource(ReweightSource const&);
    ReweightSource(char const* expr, TObject const* source = nullptr) : xexpr_(expr), yexpr_(""), source_(source) {}
    ReweightSource(char const* xexpr, char const* yexpr, TObject const* source = nullptr) : xexpr_(xexpr), yexpr_(yexpr), source_(source) {}

    ReweightPtr compile(FormulaLibrary&) const;

  private:
    TString xexpr_{""};
    TString yexpr_{""};
    TObject const* source_{nullptr};
  };

  typedef std::unique_ptr<ReweightSource> ReweightSourcePtr;

}

#endif
