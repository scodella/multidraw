#include "../interface/FormulaLibrary.h"

#include <iostream>
#include <sstream>

multidraw::FormulaLibrary::FormulaLibrary(TTree& _tree) :
  tree_(_tree)
{
}

multidraw::FormulaLibrary::~FormulaLibrary()
{
  for (auto& p : *this)
    delete p.second;
}

TTreeFormulaCached*
multidraw::FormulaLibrary::getFormula(char const* _expr)
{
  auto fItr(this->find(_expr));
  if (fItr != this->end()) {
    fItr->second->SetNRef(fItr->second->GetNRef() + 1);
    return fItr->second;
  }

  auto* formula(NewTTreeFormulaCached("formula", _expr, &tree_));
  if (formula == nullptr) {
    std::stringstream ss;
    ss << "Failed to compile expression \"" << _expr << "\"";
    std::cerr << ss.str();
    throw std::invalid_argument(ss.str());
  }

  this->emplace(_expr, formula);

  return formula;
}

void
multidraw::FormulaLibrary::deleteFormula(TTreeFormulaCached*& _formula)
{
  if (_formula == nullptr)
    return;

  if (this->count(_formula->GetTitle()) == 0) {
    std::stringstream ss;
    ss << "Formula \"" << _formula->GetTitle() << "\" not found in library";
    std::cerr << ss.str();
    throw std::invalid_argument(ss.str());
  }

  if (_formula->GetNRef() == 1) {
    this->erase(_formula->GetTitle());
    delete _formula;
    _formula = nullptr;
  }
}
