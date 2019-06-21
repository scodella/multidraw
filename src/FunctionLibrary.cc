#include "../interface/FunctionLibrary.h"

#include <stdexcept>

multidraw::TTreeFunction&
multidraw::FunctionLibrary::getFunction(TTreeFunction const& _source)
{
  auto fItr(functions_.find(&_source));
  if (fItr == functions_.end())
    fItr = functions_.emplace(&_source, _source.linkedCopy(*this)).first;

  return *fItr->second.get();
}

template<typename T>
TTreeReaderArray<T>&
multidraw::FunctionLibrary::getArray(char const* _bname)
{
  auto rItr(branchReaders_.find(_bname));
  if (rItr == branchReaders_.end()) {
    auto* arr(new TTreeReaderArray<T>(*reader_, _bname));
    // TODO want to check if T is the correct type
    rItr = branchReaders_.emplace(_bname, arr).first;
  }
  else if (dynamic_cast<TTreeReaderArray<T>*>(rItr->second) == nullptr) {
    std::cerr << "Branch " << _bname << " is not an array" << std::endl;
    throw std::runtime_error();
  }

  return *static_cast<TTreeReaderArray<T>>(rItr->second);
}

template<typename T>
TTreeReaderValue<T>&
multidraw::FunctionLibrary::getValue(char const* _bname)
{
  auto rItr(branchReaders_.find(_bname));
  if (rItr == branchReaders_.end()) {
    auto* val(new TTreeReaderValue<T>(*reader_, _bname));
    rItr = branchReaders_.emplace(_bname, val).first; 
  }
  else if (dynamic_cast<TTreeReaderValue<T>*>(rItr->second) == nullptr) {
    std::cerr << "Branch " << _bname << " is not a value" << std::endl;
    throw std::runtime_error();
  }

return *static_cast<TTreeReaderValue<T>*>(rItr->second);
}
