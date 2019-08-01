#include "../interface/FunctionLibrary.h"

multidraw::FunctionLibrary::~FunctionLibrary()
{
  for (auto& f : destructorCallbacks_)
    f();
}

void
multidraw::FunctionLibrary::setEntry(long long _iEntry)
{
  reader_->SetEntry(iEntry);
  for (auto& fct : functions_)
    fct.second->beginEvent(_iEntry);
}

multidraw::TTreeFunction&
multidraw::FunctionLibrary::getFunction(TTreeFunction const& _source)
{
  auto fItr(functions_.find(&_source));
  if (fItr == functions_.end())
    fItr = functions_.emplace(&_source, _source.linkedCopy(*this)).first;

  return *fItr->second.get();
}
