#include "../inc/Cut.h"
#include "../inc/ExprFiller.h"

#include "TTreeFormula.h"

multidraw::Cut::Cut(char const* _name, TTreeFormula& _formula) :
  name_(_name),
  formula_(&_formula)
{
}

multidraw::Cut::Cut(char const* _name, CutFunc const& _func) :
  name_(_name),
  func_(_func)
{
}

multidraw::Cut::~Cut()
{
  for (auto* filler : fillers_)
    delete filler;
}

void
multidraw::Cut::setFormula(TTreeFormula& _formula)
{
  func_ = nullptr;
  formula_ = &_formula;
}
