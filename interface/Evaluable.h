#ifndef multidraw_Evaluable_h
#define multidraw_Evaluable_h

#include "TTreeFormula.h"

#include <functional>

namespace multidraw {

  class Evaluable {
  public:
    typedef std::function<Double_t(UInt_t)> InstanceVal;
    typedef std::function<UInt_t(void)> NData;

    Evaluable() {}
    Evaluable(InstanceVal const&, NData const& = nullptr);
    Evaluable(TTreeFormula&);
    Evaluable(Evaluable const&);
    ~Evaluable() {}
    Evaluable& operator=(Evaluable const&);

    void set(InstanceVal const&, NData const& = nullptr);
    void set(TTreeFormula&);

    Bool_t singleValue() const { return singleValue_; }
    UInt_t getNdata() const { return ndata_(); }
    Double_t evalInstance(UInt_t i) const { return instanceVal_(i); }

  private:
    InstanceVal instanceVal_{};
    NData ndata_{};
    Bool_t singleValue_{false};
  };

}

#endif
