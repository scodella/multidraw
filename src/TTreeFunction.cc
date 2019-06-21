#include "../interface/TTreeFunction.h"

multidraw::TTreeFunctionPtr
multidraw::TTreeFunction::linkedCopy(FunctionLibrary& _library) const
{
  auto* clone{clone_()};
  clone->bindTree_(_library);
  clone->linked_ = true;

  return TTreeFunctionPtr(clone);
}
