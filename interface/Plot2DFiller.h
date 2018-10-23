#ifndef multidraw_Plot2DFiller_h
#define multidraw_Plot2DFiller_h

#include "TTreeFormulaCached.h"
#include "ExprFiller.h"

#include "TH2.h"

namespace multidraw {

  //! A wrapper class for TH2
  /*!
   * The class is to be used within MultiDraw, and is instantiated by addPlot().
   * Arguments:
   *  hist     The actual histogram object (the user is responsible for creating it)
   *  xexpr    Expression whose evaluated value gets filled to the plot
   *  yexpr    Expression whose evaluated value gets filled to the plot
   *  reweight If provided, evalutaed and used as weight for filling the histogram
   */
  class Plot2DFiller : public ExprFiller {
  public:
    Plot2DFiller(TH2& hist, TTreeFormulaCachedPtr const& xexpr, TTreeFormulaCachedPtr const& yexpr, TTreeFormulaCachedPtr const& reweight = nullptr);
    Plot2DFiller(Plot2DFiller const&);
    ~Plot2DFiller() {}

    TObject const& getObj() const override { return hist_; }
    TH2 const& getHist() const { return hist_; }

  private:
    void doFill_(unsigned) override;

    TH2& hist_;
  };

}

#endif
