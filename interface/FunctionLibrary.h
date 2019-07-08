#ifndef multidraw_TTreeReaderLibrary_h
#define multidraw_TTreeReaderLibrary_h

#include "TTreeFunction.h"

#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"
#include "TString.h"

#include <unordered_map>
#include <memory>

typedef std::unique_ptr<ROOT::Internal::TTreeReaderValueBase> TTreeReaderValueBasePtr;

namespace multidraw {

  class FunctionLibrary {
  public:
    FunctionLibrary(TTree& tree) : reader_(new TTreeReader(&tree)) {}
    ~FunctionLibrary() {}

    void setEntry(long long iEntry) { reader_->SetEntry(iEntry); }

    TTreeFunction& getFunction(TTreeFunction const&);

    template<typename T> TTreeReaderArray<T>& getArray(char const*);
    template<typename T> TTreeReaderValue<T>& getValue(char const*);

  private:
    std::unique_ptr<TTreeReader> reader_{};
    std::unordered_map<std::string, TTreeReaderValueBasePtr> branchReaders_{};
    std::unordered_map<TTreeFunction const*, std::unique_ptr<TTreeFunction>> functions_{};
  };
  
}

#include <stdexcept>
#include <sstream>

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
  else if (dynamic_cast<TTreeReaderArray<T>*>(rItr->second.get()) == nullptr) {
    std::stringstream ss;
    ss << "Branch " << _bname << " is not an array" << std::endl;
    throw std::runtime_error(ss.str());
  }

  return *static_cast<TTreeReaderArray<T>*>(rItr->second.get());
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
  else if (dynamic_cast<TTreeReaderValue<T>*>(rItr->second.get()) == nullptr) {
    std::stringstream ss;
    ss << "Branch " << _bname << " is not a value" << std::endl;
    throw std::runtime_error(ss.str());
  }

  return *static_cast<TTreeReaderValue<T>*>(rItr->second.get());
}

typedef TTreeReaderArray<Float_t> FloatArrayReader;
typedef TTreeReaderValue<Float_t> FloatValueReader;
typedef TTreeReaderArray<Int_t> IntArrayReader;
typedef TTreeReaderValue<Int_t> IntValueReader;
typedef TTreeReaderArray<Bool_t> BoolArrayReader;
typedef TTreeReaderValue<Bool_t> BoolValueReader;
typedef TTreeReaderArray<UChar_t> UCharArrayReader;
typedef TTreeReaderValue<UChar_t> UCharValueReader;
typedef TTreeReaderArray<UInt_t> UIntArrayReader;
typedef TTreeReaderValue<UInt_t> UIntValueReader;
typedef TTreeReaderValue<ULong64_t> ULong64ValueReader;
typedef std::unique_ptr<FloatArrayReader> FloatArrayReaderPtr;
typedef std::unique_ptr<FloatValueReader> FloatValueReaderPtr;
typedef std::unique_ptr<IntArrayReader> IntArrayReaderPtr;
typedef std::unique_ptr<IntValueReader> IntValueReaderPtr;
typedef std::unique_ptr<BoolArrayReader> BoolArrayReaderPtr;
typedef std::unique_ptr<BoolValueReader> BoolValueReaderPtr;
typedef std::unique_ptr<UCharArrayReader> UCharArrayReaderPtr;
typedef std::unique_ptr<UCharValueReader> UCharValueReaderPtr;
typedef std::unique_ptr<UIntArrayReader> UIntArrayReaderPtr;
typedef std::unique_ptr<UIntValueReader> UIntValueReaderPtr;
typedef std::unique_ptr<ULong64ValueReader> ULong64ValueReaderPtr;

#endif
