#include "../interface/FormulaLibrary.h"

#include <iostream>
#include <sstream>

multidraw::FormulaLibrary::FormulaLibrary(TTree& _tree) :
  tree_(_tree)
{
}

TTreeFormulaCachedPtr const&
multidraw::FormulaLibrary::getFormula(char const* _expr, bool _silent/* = false*/)
{
  auto fItr(this->find(_expr));
  if (fItr != this->end()) {
    return fItr->second;
  }

  auto* formula(NewTTreeFormulaCached("formula", _expr, &tree_, _silent));
  if (formula == nullptr) {
    std::stringstream ss;
    ss << "Failed to compile expression \"" << _expr << "\"";
    if (!_silent)
      std::cerr << ss.str();
    throw std::invalid_argument(ss.str());
  }

  fItr = this->emplace(TString(_expr), TTreeFormulaCachedPtr(formula)).first;

  return fItr->second;
}

void
multidraw::FormulaLibrary::prune()
{
  auto fItr(this->begin());
  while (fItr != this->end()) {
    auto next(fItr);
    ++next;

    if (fItr->second.use_count() == 1)
      this->erase(fItr);

    fItr = next;
  }
}
