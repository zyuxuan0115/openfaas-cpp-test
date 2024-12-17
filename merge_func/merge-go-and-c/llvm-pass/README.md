### Get the correct version for gollvm
```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout 09629215c272f09e3ebde6cc7eac9625d28910ff
cd llvm/tools
git clone https://go.googlesource.com/gollvm
cd gollvm
git checkout 253c122ed62d5e9a32a9806e83c47a389a6435bf
git clone https://go.googlesource.com/gofrontend
cd gofrontend
git checkout 50707b4b51266166ce9bcf9de187e35760ec50f9
cd ../libgo
git clone https://github.com/libffi/libffi.git
cd libffi
git checkout aa3fce08ba620c50db17215a9f14dd0f1facf741
cd ../
git clone https://github.com/ianlancetaylor/libbacktrace.git
cd libbacktrace
git checkout 2446c66076480ce07a6bd868badcbceb3eeecc2e
```

### Build gollvm

```bash
cmake -DCMAKE_INSTALL_PREFIX=/proj/zyuxuanssf-PG0/gollvm \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_USE_LINKER=gold \
  -G Ninja ../llvm
ninja gollvm
ninja install-gollvm
export LD_LIBRARY_PATH=/proj/zyuxuanssf-PG0/gollvm/lib64:$LD_LIBRARY_PATH
export PATH=/proj/zyuxuanssf-PG0/gollvm/bin:$PATH
```

### Generate LLVM IR for a go program

```bash
# can generate LLVM IR only based on this
> llvm-goc -emit-llvm -S -o caller.ll caller.go
# disable function inlining
> llvm-goc -O0 -fno-inline -emit-llvm -S -o caller.ll caller.go
# generate binary 
> llc -filetype=obj caller.ll -o caller.o
> clang caller.o \
  $GOLLVM/lib64/libgobegin.a \
  $GOLLVM/lib64/libgolibbegin.a \
  $GOLLVM/lib64/libgo.a \
  -o caller -lpthread -lm
```

### add MergeRustFuncAsync pass
```bash
> cp *.h llvm-project/llvm/include/llvm/Transforms/Utils/MergeRustFuncAsync.h
> cp *.cpp llvm-project/llvm/lib/Transforms/Utils/MergeRustFuncAsync.cpp
```

- In `llvm-project/llvm/lib/Transforms/Utils/CMakeLists.txt` add `MergeRustFuncAsync.cpp`
- In `llvm-project/llvm/lib/Passes/PassRegistry.def` add `MODULE_PASS("merge-rust-func-async", MergeRustFuncAsyncPass())` 
- In `llvm-project/llvm/lib/Passes/PassBuilder.cpp` add `#include "llvm/Transforms/Utils/MergeRustFuncAsync.h"`

### to run the optimization pass
```bash
> llvm-project/build/bin/opt -disable-output main.ll -passes=merge-rust-func-async
```