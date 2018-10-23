#ifndef multidraw_Evaluable_h
#define multidraw_Evaluable_h

#include <functional>
#include <memory>

class TTreeFormulaCached;

namespace multidraw {

  class Evaluable {
  public:
    typedef std::function<double(unsigned)> InstanceVal;
    typedef std::function<unsigned(void)> NData;

    Evaluable() {}
    Evaluable(InstanceVal const&, NData const& = nullptr);
    Evaluable(std::shared_ptr<TTreeFormulaCached> const&);
    Evaluable(Evaluable const&);
    ~Evaluable() {}
    Evaluable& operator=(Evaluable const&);

    void set(InstanceVal const&, NData const& = nullptr);
    void set(std::shared_ptr<TTreeFormulaCached> const&);
    void reset();

    bool isValid() const { return isFormula() || isFunction(); }
    bool isFormula() const { return bool(formula_); }
    bool isFunction() const { return bool(instanceVal_); }
    bool singleValue() const;
    unsigned getNdata();
    double evalInstance(unsigned i);

  private:
    InstanceVal instanceVal_{};
    NData ndata_{};
    std::shared_ptr<TTreeFormulaCached> formula_{};
    bool singleValue_{false};
  };

}

#endif
