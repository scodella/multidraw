#ifndef multidraw_Plot1DFiller_h
#define multidraw_Plot1DFiller_h

#include "TTreeFormulaCached.h"
#include "ExprFiller.h"

#include "TH1.h"

namespace multidraw {

  //! A wrapper class for TH1
  /*!
   * The class is to be used within MultiDraw, and is instantiated by addPlot().
   * Arguments:
   *  hist      The actual histogram object (the user is responsible for creating it)
   *  expr      Expression whose evaluated value gets filled to the plot
   *  reweight  If provided, evalutaed and used as weight for filling the histogram
   *  mode      kDefault: overflow is not handled explicitly (i.e. TH1 fills the n+1-st bin)
   *            kDedicated: an overflow bin with size (original width)*overflowBinSize is created
   *            kMergeLast: overflow is added to the last bin
   */
  class Plot1DFiller : public ExprFiller {
  public:
    enum OverflowMode {
      kDefault,
      kDedicated,
      kMergeLast
    };

    Plot1DFiller(TH1& hist, TTreeFormulaCachedPtr const& expr, TTreeFormulaCachedPtr const& reweight = nullptr, OverflowMode mode = kDefault);
    Plot1DFiller(Plot1DFiller const&);
    ~Plot1DFiller();

    TObject const& getObj() const override { return hist_; }
    TObject& getObj() override { return hist_; }
    TH1 const& getHist() const { return hist_; }

    ExprFiller* threadClone(FormulaLibrary&) const override;
    void threadMerge(TObject&) override;

  private:
    void doFill_(unsigned) override;

    TH1& hist_;
    OverflowMode overflowMode_{kDefault};
  };

}

#endif
