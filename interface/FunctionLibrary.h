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

    TTreeFunction& getFunction(TTreeFunction const&);

    template<typename T> TTreeReaderArray<T>& getArray(char const*);
    template<typename T> TTreeReaderValue<T>& getValue(char const*);

  private:
    std::unique_ptr<TTreeReader> reader_{};
    std::unordered_map<std::string, TTreeReaderValueBasePtr> branchReaders_{};
    std::unordered_map<TTreeFunction const*, std::unique_ptr<TTreeFunction>> functions_{};
  };

}

#endif
