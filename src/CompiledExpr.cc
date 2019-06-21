#include "../interface/CompiledExpr.h"

multidraw::CompiledExpr::CompiledExpr(TTreeFunction& _function) :
  function_(&_function)
{
  if (!function_->isLinked())
    throw std::runtime_error("Unlinked TTreeFunction used to construct CompiledExpr");
}

unsigned
multidraw::CompiledExpr::getNdata()
{
  if (formula_ != nullptr)
    return formula_->GetNdata();
  else
    return function_->getNdata();
}

double
multidraw::CompiledExpr::evaluate(unsigned _iD)
{
  if (formula_ != nullptr)
    return formula_->EvalInstance(_iD);
  else
    return function_->evaluate(_iD);
}
