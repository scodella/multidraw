#ifndef multidraw_TTreeFunction_h
#define multidraw_TTreeFunction_h

#include <memory>

namespace multidraw {

  class FunctionLibrary;

  class TTreeFunction {
  public:
    TTreeFunction() {}
    virtual ~TTreeFunction() {}

    virtual std::unique_ptr<TTreeFunction> linkedCopy(FunctionLibrary&) const = 0;
  
    virtual unsigned getNdata() = 0;
    virtual double evaluate(unsigned) = 0;
  };

}

#endif
