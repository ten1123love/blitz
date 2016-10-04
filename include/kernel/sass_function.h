#ifndef INCLUDE_KERNELS_SASS_FUNCTION_H_
#define INCLUDE_KERNELS_SASS_FUNCTION_H_

#include <cuda.h>
#include <boost/thread/once.hpp>
#include <boost/noncopyable.hpp>

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "util/common.h"

namespace blitz {

class CubinLoadModule {
 public:
  CubinLoadModule() {
    // init kernels list
    const size_t kernel_size = 6;
    const string kernel_name[kernel_size] = {
      "sgemm_nn_128x128",
      "sgemm_nt_128x128",
      "sgemm_tn_128x128",
      "sgemm_nn_128x128_vec",
      "sgemm_nt_128x128_vec",
      "sgemm_tn_128x128_vec",
    };

    for (size_t i = 0; i < kernel_size; ++i) {
      const string& name = kernel_name[i];
      const string path = "./cubin/" + name + ".cubin";

      CUmodule module;
      CUfunction function;
      CUresult res;

      // load module
      res = cuModuleLoad(&module, path.c_str());
      if (res != CUDA_SUCCESS) {
        LOG(FATAL) << "Failed to load module: " << name << std::endl;
      }

      // load function
      res = cuModuleGetFunction(&function, module, name.c_str());
      if (res != CUDA_SUCCESS) {
        LOG(FATAL) << "Failed to load function: " << name << std::endl;
      }

      functions_[name] = function;
      modules_.push_back(module);
    }
  }

  ~CubinLoadModule() {
    typedef vector<CUmodule>::iterator ModuleIterator;
    for (ModuleIterator it = modules_.begin(); it != modules_.end(); ++it) {
      cuModuleUnload(*it);
    }
  }

  CUfunction GetFunction(const string& name) {
    if (functions_.find(name) == functions_.end()) {
      LOG(FATAL) << "Cannot find kernel: " << name;
    }

    return functions_[name];
  }

 private:
  map<string, CUfunction> functions_;
  vector<CUmodule> modules_;

  DISABLE_COPY_AND_ASSIGN(CubinLoadModule);
};

class CubinModule {
 public:
  static CubinLoadModule& GetInstance() {
    // thread safe
    boost::call_once(&CubinModule::Create, flag_);
    return *(CubinModule::instance_);
  }

  static void Create() {
    CubinModule::instance_.reset(new CubinLoadModule());
  }

  static CUfunction GetFunction(const string& name) {
    CubinLoadModule& cubin_load_module = CubinModule::GetInstance();
    return cubin_load_module.GetFunction(name);
  }

  virtual ~CubinModule();

 private:
  CubinModule();

  static scoped_ptr<CubinLoadModule> instance_;
  static boost::once_flag flag_;

  DISABLE_COPY_AND_ASSIGN(CubinModule);
};

template<typename DType>
void BlitzSassGemm(const bool transa, const bool transb,
  const int M, const int N, const int K,
  DType* A, DType* B, DType* C, DType alpha, DType beta);

template<typename DType>
void BlitzSass2DConvolution(const int batch_size,
  const int input_channel, const int input_height,
  const int input_width, const int filter_height,
  const int filter_width, const int output_channel,
  const int output_height, const int output_width,
  const int stride_height, const int stride_width,
  DType* input, DType* output, DType* filter,
  const string& phase);
}  // namespace blitz

#endif  // INCLUDE_KERNELS_SASS_FUNCTION_H_