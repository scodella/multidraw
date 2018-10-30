#ifndef multidraw_Evaluable_h
#define multidraw_Evaluable_h

#include "TTreeFormulaCached.h"

#include <functional>

namespace multidraw {

  class Evaluable {
  public:
    typedef std::function<double(unsigned)> InstanceVal;
    typedef std::function<unsigned(void)> NData;

    Evaluable() {}
    Evaluable(InstanceVal const&, NData const& = nullptr);
    Evaluable(TTreeFormulaCachedPtr const&);
    Evaluable(Evaluable const&);
    ~Evaluable() {}
    Evaluable& operator=(Evaluable const&);

    void set(InstanceVal const&, NData const& = nullptr);
    void set(TTreeFormulaCachedPtr const&);
    void reset();

    TString getExpression() const;
    InstanceVal const& getInstanceVal() const { return instanceVal_; }
    NData const& getNData() const { return ndata_; }

    bool isValid() const { return isFormula() || isFunction(); }
    bool isFormula() const { return bool(formula_); }
    bool isFunction() const { return bool(instanceVal_); }
    bool singleValue() const;
    unsigned getNdata();
    double evalInstance(unsigned i);

  private:
    InstanceVal instanceVal_{};
    NData ndata_{};
    TTreeFormulaCachedPtr formula_{};
    bool singleValue_{false};
  };

}

#endif
