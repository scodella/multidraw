#ifndef multidraw_MultiDraw_h
#define multidraw_MultiDraw_h

#include "ExprFiller.h"
#include "TTreeFormulaCached.h"
#include "Flags.h"

#include "TChain.h"
#include "TH1.h"
#include "TString.h"

#include <map>
#include <vector>

namespace multidraw {

  //! A handy class to fill multiple histograms in one pass over a TChain using string expressions.
  /*!
   * Usage
   *  MultiDraw drawer;
   *  drawer.addInputPath("source.root");
   *  drawer.setBaseSelection("electrons.loose && TMath::Abs(electrons.eta_) < 1.5");
   *  drawer.setFullSelection("electrons.tight");
   *  TH1D* h1 = new TH1D("h1", "histogram 1", 100, 0., 100.);
   *  TH1D* h2 = new TH1D("h2", "histogram 2", 100, 0., 100.);
   *  drawer.addPlot(h1, "electrons.pt_", "", true, false);
   *  drawer.addPlot(h2, "electrons.pt_", "", true, true);
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
    void setWeightBranch(char const* bname, char type = 'F') { weightBranchName_ = bname; }
    //! Set the baseline selection.
    void setBaseSelection(char const* cuts);
    //! Set the full selection.
    void setFullSelection(char const* cuts);
    //! Apply a constant weight (e.g. luminosity times cross section) to all events.
    void setConstantWeight(double l) { constWeight_ = l; }
    //! Set a prescale factor
    /*!
     * When prescale_ > 1, only events that satisfy eventNumber % prescale_ == 0 are read.
     */
    void setPrescale(unsigned p) { prescale_ = p; }
    //! Set a global reweight
    /*!
     * Reweight factor can be set in two ways. If the second argument is nullptr,
     * the value of expr in every event is used as the weight. If instead a TH1,
     * TGraph, or TF1 is passed as the second argument, the value of expr is used
     * to look up the y value of the source object, which is used as the weight.
     */
    void setReweight(char const* expr, TObject const* source = nullptr);
    //! Add a histogram to fill.
    /*!
     * Currently only 1D histograms can be used.
     */
    void addPlot(TH1* hist, char const* expr, char const* cuts = "", bool applyBaseline = true, bool applyFullSelection = false, char const* reweight = "", Plot1DOverflowMode mode = kDefault);
    //! Add a tree to fill.
    /*!
     * Use addTreeBranch to add branches.
     */
    void addTree(TTree* tree, char const* cuts = "", bool applyBaseline = true, bool applyFullSelection = false, char const* reweight = "");
    //! Add a branch to a tree already added to the MultiDraw object.
    void addTreeBranch(TTree* tree, char const* bname, char const* expr);

    //! Run and fill the plots and trees.
    void fillPlots(long nEntries = -1, long firstEntry = 0, char const* filter = nullptr);

    void setPrintLevel(int l) { printLevel_ = l; }
    long getTotalEvents() { return totalEvents_; }

    unsigned numObjs() const { return unconditional_.size() + postBase_.size() + postFull_.size(); }

  private:
    //! Handle addPlot and addTree with the same interface (requires a callback to generate the right object)
    typedef std::function<ExprFiller*(TTreeFormula*, TTreeFormula*)> ObjGen;
    void addObj_(char const* cuts, bool applyBaseline, bool applyFullSelection, char const* reweight, ObjGen const&);

    TTreeFormulaCached* getFormula_(char const*);
    void deleteFormula_(TTreeFormulaCached*);

    TChain tree_;
    TString weightBranchName_{"weight"};
    TTreeFormulaCached* baseSelection_{nullptr};
    TTreeFormulaCached* fullSelection_{nullptr};
    double constWeight_{1.};
    unsigned prescale_{1};
    TTreeFormulaCached* reweightExpr_{nullptr};
    std::function<void(std::vector<double>&)> reweight_;
    std::vector<ExprFiller*> unconditional_{};
    std::vector<ExprFiller*> postBase_{};
    std::vector<ExprFiller*> postFull_{};

    std::map<TString, TTreeFormulaCached*> library_;

    int printLevel_{0};
    long totalEvents_{0};
  };

}

#endif
