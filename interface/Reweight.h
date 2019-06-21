#ifndef multidraw_Reweight_h
#define multidraw_Reweight_h

#include "TTreeFormulaCached.h"

#include "TH1.h"
#include "TGraph.h"
#include "TF1.h"
#include "TSpline.h"

#include <functional>

class TSpline3;

namespace multidraw {

  class Reweight {
  public:
    Reweight() {}
    Reweight(TTreeFormulaCached*, TTreeFormulaCached* = nullptr);
    Reweight(TObject const&, TTreeFormulaCached*, TTreeFormulaCached* = nullptr, TTreeFormulaCached* = nullptr);
    virtual ~Reweight() {}

    //! Set the tree formula to reweight with.
    void set(TTreeFormulaCached* xformula, TTreeFormulaCached* yformula = nullptr);
    //! Reweight through a histogram, a graph, or a function. Formulas return the x, y, z values to be fed to the object.
    void set(TObject const&, TTreeFormulaCached*, TTreeFormulaCached* = nullptr, TTreeFormulaCached* = nullptr);

    virtual void reset();

    virtual unsigned getNdim() const { return formulas_.size(); }
    virtual TTreeFormulaCached* getFormula(unsigned i = 0) const { return formulas_.at(i); }
    virtual TObject const* getSource(unsigned i = 0) const { return source_; }

    virtual unsigned getNdata();
    virtual double evaluate(unsigned i = 0) const { if (evaluate_) return evaluate_(i); else return 1.; }

  protected:
    void setRaw_();
    void setTH1_();
    void setTGraph_();
    void setTF1_();

    double evaluateRaw_(unsigned);
    double evaluateTH1_(unsigned);
    double evaluateTGraph_(unsigned);
    double evaluateTF1_(unsigned);

    //! One entry per source dimension
    std::vector<TTreeFormulaCached*> formulas_{};
    TObject const* source_{nullptr};
    std::unique_ptr<TSpline3> spline_{};

    std::function<double(unsigned)> evaluate_{};
  };

  typedef std::unique_ptr<Reweight> ReweightPtr;

  class FactorizedReweight : public Reweight {
  public:
    FactorizedReweight(ReweightPtr&&, ReweightPtr&&); // combined (factorized) reweighting
    ~FactorizedReweight() {}
    //! Reweight through a combination of reweights
    void set(ReweightPtr&&, ReweightPtr&&);

    void reset() override;

    unsigned getNdim() const override { return subReweights_[0]->getNdim() * subReweights_[1]->getNdim(); }
    TTreeFormulaCached* getFormula(unsigned i = 0) const override;
    TObject const* getSource(unsigned i = 0) const override { return subReweights_[i]->getSource(); }

    unsigned getNdata() override { return subReweights_[0]->getNdata(); }
    double evaluate(unsigned i = 0) const override { return subReweights_[0]->evaluate(i) * subReweights_[1]->evaluate(i); }

  private:
    std::array<ReweightPtr, 2> subReweights_{};
  };

  class FormulaLibrary;

  class ReweightSource {
  public:
    ReweightSource() {}
    ReweightSource(ReweightSource const&);
    ReweightSource(char const* expr, TObject const* source = nullptr) : xexpr_(expr), yexpr_(""), source_(source) {}
    ReweightSource(char const* xexpr, char const* yexpr, TObject const* source = nullptr) : xexpr_(xexpr), yexpr_(yexpr), source_(source) {}
    ReweightSource(ReweightSource const& r1, ReweightSource const& r2) {
      subReweights_[0] = std::make_unique<ReweightSource>(r1);
      subReweights_[1] = std::make_unique<ReweightSource>(r2);
    }

    ReweightPtr compile(FormulaLibrary&) const;

  private:
    TString xexpr_{""};
    TString yexpr_{""};
    TObject const* source_{nullptr};

    std::array<std::unique_ptr<ReweightSource>, 2> subReweights_;
  };

  typedef std::unique_ptr<ReweightSource> ReweightSourcePtr;

}

#endif
