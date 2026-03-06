# tiny-c2ll

Super tiny C-subset compiler that outputs LLVM IR text without linking LLVM libraries.

## Build With CMake

```bash
cmake -S . -B build
cmake --build build
```

This generates `build/example.ll` through the `build_example_ir` target.

Optional executable build (requires `clang`):

```bash
cmake --build build --target build_example_bin
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

For parallel test execution:

```bash
ctest --test-dir build --output-on-failure -j 8
```

## Progress Scripts

```bash
bash scripts/check_progress.sh
bash scripts/full_scan.sh
```

Both scripts use CMake + CTest and run `c_testsuite` tests in parallel.

### Optional: Run `c-testsuite` Allowlist

1. Clone `c-testsuite` locally (any path you choose).
2. Add supported test files into `tests/c_testsuite_allowlist.txt`.
3. Configure with testsuite root:

```bash
# first time to init
git submodule update --init tests/c-testsuite
# in project root
cmake -S . -B build -DC_TESTSUITE_ROOT=$PWD/tests/c-testsuite
cmake --build build
ctest --test-dir build --output-on-failure -L c_testsuite -j 8
```

If `tests/c-testsuite` exists in this repo, CMake auto-enables it without `-DC_TESTSUITE_ROOT`.

### Optional: Run `llvm-test-suite` `gcc-c-torture` Allowlist

1. Clone `llvm-test-suite` locally (any path you choose), or initialize this repo submodule.
2. Add supported test files into `tests/llvm_gcc_c_torture_allowlist.txt`.
3. Configure with testsuite root:

```bash
# first time to init
git submodule update --init tests/llvm-test-suite
# in project root
cmake -S . -B build \
  -DLLVM_TEST_SUITE_ROOT=$PWD/tests/llvm-test-suite \
  -DLLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC=20 \
  -DLLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC=90 \
  -DLLVM_GCC_C_TORTURE_RUN_MEM_MB=1024 \
  -DLLVM_GCC_C_TORTURE_RUN_CPU_SEC=20
cmake --build build
ctest --test-dir build --output-on-failure -L llvm_gcc_c_torture -j 8
```

If `tests/llvm-test-suite` exists in this repo, CMake auto-enables it without `-DLLVM_TEST_SUITE_ROOT`.

`llvm_gcc_c_torture` knobs:

- `LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC`: timeout per compile/run step inside each case script.
- `LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC`: ctest timeout for a whole case.
- `LLVM_GCC_C_TORTURE_RUN_MEM_MB`: runtime memory cap for test binaries (0 = disabled, best-effort via `ulimit`).
- `LLVM_GCC_C_TORTURE_RUN_CPU_SEC`: runtime CPU seconds cap for test binaries (0 = disabled, best-effort via `ulimit`).

After a full run, keep only failed cases in allowlist (remove passes) for faster iterations:

```bash
cmake --build build --target prune_llvm_gcc_c_torture_allowlist_to_failed
```

This rewrites `tests/llvm_gcc_c_torture_allowlist.txt` and saves a backup at
`tests/llvm_gcc_c_torture_allowlist.txt.bak`.

Fast fix loop for agent (stop at first failure, then shrink allowlist to failed):

```bash
./scripts/check_progress_llvm_gcc_c_torture.sh
```

Useful env vars:

- `BUILD_DIR` (default `build_debug`)
- `LLVM_TEST_SUITE_ROOT` (default `tests/llvm-test-suite`)
- `STOP_ON_FAILURE` (default `1`)
- `PRUNE_FAILED_ALLOWLIST` (default `1`)
- `STEP_TIMEOUT_SEC`, `TEST_TIMEOUT_SEC`, `RUN_MEM_MB`, `RUN_CPU_SEC`


### Optional: Build Linux Kernel (tinyconfig)

Install required packages first:

```bash
sudo apt-get update
sudo apt-get install -y flex bison bc libssl-dev
```

Use the helper script:

```bash
bash scripts/linux_init.sh
```

What it does:

1. If `tests/linux` is a git-tracked submodule, initialize it recursively.
2. Build `frontend_cxx_stage1` and `frontend_cxx_stage2` into `build_linux` by default.
3. Generate kernel `tinyconfig` under `tests/linux-build` with `CC=clang`.

After init completes, run the final kernel build command printed by the script, for example:

```bash
make -C tests/linux O=tests/linux-build CC=build_linux/tiny-c2ll-stage2 -j"$(nproc)"
```

Useful environment variables:

- `BUILD_DIR` (default: `build_linux`)
- `KERNEL_SRC` (default: `tests/linux`)
- `KERNEL_OUT` (default: `tests/linux-build`)
- `JOBS` (default: detected CPU count)


## Manual Usage

```bash
./build/tiny-c2ll-stage1 tests/tiny_c2ll/example.c -o /tmp/out.ll
clang /tmp/out.ll -o /tmp/out
/tmp/out
echo $?
```

`echo $?` prints the C `main` return value.

## Pipeline

- Lexer: source text -> token stream
- Parser: token stream -> AST
- SemanticAnalyzer: validates declarations, assignments, and uninitialized reads
- IRBuilder: AST -> LLVM IR text

## Next

- Language roadmap and pattern-matching design: `docs/SPEC.md`

## Run agent

```bash
caffeinate -dimsu ./scripts/run_agent.sh
```
