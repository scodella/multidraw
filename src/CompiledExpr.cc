#include "../interface/CompiledExpr.h"

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
