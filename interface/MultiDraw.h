#ifndef multidraw_MultiDraw_h
#define multidraw_MultiDraw_h

#include "TTreeFormulaCached.h"
#include "Flags.h"
#include "ExprFiller.h"
#include "Plot1DFiller.h"
#include "TreeFiller.h"
#include "Cut.h"
#include "FormulaLibrary.h"

#include "TChain.h"
#include "TH1.h"
#include "TString.h"

#include <vector>

namespace multidraw {

  //! A handy class to fill multiple histograms in one pass over a TChain using string expressions.
  /*!
   * Usage
   *  MultiDraw drawer;
   *  drawer.addInputPath("source.root");
   *  drawer.addCut("baseline", "electrons.loose && TMath::Abs(electrons.eta_) < 1.5");
   *  drawer.addCut("fullsel", "electrons.tight");
   *  TH1D* h1 = new TH1D("h1", "histogram 1", 100, 0., 100.);
   *  TH1D* h2 = new TH1D("h2", "histogram 2", 100, 0., 100.);
   *  drawer.addPlot(h1, "electrons.pt_", "", "baseline");
   *  drawer.addPlot(h2, "electrons.pt_", "", "fullsel");
   *  drawer.fillPlots();
   *
   * In the above example, histogram h1 is filled with the pt of the loose barrel electrons, and
   * h2 with the subset of such electrons that also pass the tight selection.
   * It is also possible to set cuts and reweights for individual plots. Event-wide weights can
   * be set by three methods setWeightBranch, setConstantWeight, and setGlobalReweight.
   */
  class MultiDraw {
  public:
    //! Constructor.
    /*!
     * \param treeName  Name of the input tree.
     */
    MultiDraw(char const* treeName = "events");
    ~MultiDraw();

    //! Add an input file.
    void addInputPath(char const* path) { tree_.Add(path); }
    //! Set the name and the C variable type of the weight branch. Pass an empty string to unset.
    void setWeightBranch(char const* bname) { weightBranchName_ = bname; }
    //! Set a global filtering cut. Events not passing this expression are not considered at all.
    void setFilter(char const* expr);
    //! Add a new cut.
    void addCut(char const* name, char const* expr);
    //! Apply a constant weight (e.g. luminosity times cross section) to all events.
    void setConstantWeight(double l) { constWeight_ = l; }
    //! Set a prescale factor
    /*!
     * When prescale_ > 1, only events that satisfy eventNumber % prescale_ == 0 are read.
     * If evtNumBranch is set, branch of the given name is read as the eventNumber. The
     * tree entry number is used otherwise.
     */
    void setPrescale(unsigned p, char const* evtNumBranch = "");
    //! Set a global reweight
    /*!
     * Reweight factor can be set in two ways. If the second argument is nullptr,
     * the value of expr in every event is used as the weight. If instead a TH1,
     * TGraph, or TF1 is passed as the second argument, the value of expr is used
     * to look up the y value of the source object, which is used as the weight.
     */
    void setReweight(char const* expr, TObject const* source = nullptr);
    //! Add a 1D histogram to fill.
    Plot1DFiller& addPlot(TH1* hist, char const* expr, char const* cutName = "", char const* reweight = "", Plot1DOverflowMode mode = kDefault);
    //! Add a tree to fill.
    TreeFiller& addTree(TTree* tree, char const* cutName = "", char const* reweight = "");

    //! Run and fill the plots and trees.
    void execute(long nEntries = -1, long firstEntry = 0);

    void setPrintLevel(int l) { printLevel_ = l; }
    long getTotalEvents() { return totalEvents_; }

    unsigned numObjs() const;

  private:
    //! Handle addPlot and addTree with the same interface (requires a callback to generate the right object)
    typedef std::function<ExprFiller*(TTreeFormula*)> ObjGen;
    ExprFiller& addObj_(TString const& cutName, char const* reweight, ObjGen const&);
    Cut& findCut_(TString const& cutName) const;

    TChain tree_;
    TString weightBranchName_{"weight"};
    TString evtNumBranchName_{""};

    unsigned prescale_{1};

    std::vector<Cut*> cuts_;
    FormulaLibrary library_;

    double constWeight_{1.};

    TTreeFormulaCached* reweightExpr_{nullptr};
    std::function<void(std::vector<double>&)> reweight_;

    int printLevel_{0};
    long totalEvents_{0};
  };

}

#endif
