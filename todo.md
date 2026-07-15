# Current Packet

Status: Active
Source Idea Path: ideas/open/802_project_wide_cpp20_host_toolchain_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit host-standard ownership and capture baseline

## Just Finished

- Plan Step 1 completed the host-standard ownership audit and classified every
  versioned build-input declaration found by repository-wide search.
- Production declarations are the root `c4c_target_profile`, `c4cll`, and
  `c4c-objdump` targets plus `c4c_frontend`, `c4c_codegen`, and `c4c_backend`.
  They currently mix four `cxx_std_17` feature declarations with two local
  `CXX_STANDARD 17` properties.
- Native-test declarations are the preprocessor executable, the frontend test
  factory, all six BIR executables, and both MIR test factories. They use local
  `CXX_STANDARD 17` properties; the two MIR factories also cover source-audit
  executables that intentionally do not link a production library.
- The manual `-std=gnu++20` flags in `tests/c/internal/cmake` and
  `tests/cpp/{eastl,external}` compile language-under-test/workflow inputs and
  are not host-target standard authorities. No versioned third-party CMake
  target declares C++17. Historical docs and the repo-local clang-tool skill
  contain example `--std=c++17` invocations, not build inputs.
- The configured acceptance host is Homebrew LLVM clang 22.1.7 on
  arm64-apple-darwin with libc++ `_LIBCPP_VERSION=220107`. Its C++20 mode
  reports `__cplusplus=202002L` and provides designated initialization,
  `consteval`, and `std::span`. Generated commands contain 59 `-std=gnu++17`
  occurrences, confirming both the old standard and extension-mode drift.
- The exact pre-change baseline was
  `cmake --preset default -DENABLE_C4C_BACKEND=ON && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -E 'llvm_gcc_c_torture' > test_before.log`:
  configure/build succeeded; 1312 tests ran, 1272 passed, and 40 failed.

## Suggested Next

- Execute plan Step 2 by establishing the root CMake-3.20-compatible authority
  with required C++20 and extensions disabled, removing every in-scope local
  C++17 pin, and verifying representative production and native-test commands
  use `-std=c++20` rather than `gnu++20`.

## Watchouts

- Keep c4cll language-under-test modes distinct from host compilation.
- Do not touch idea 732.
- `c4c_target_profile` is not a sufficient project-wide standard authority:
  `c4c-objdump` and source-audit MIR tests do not consume it, and forcing an
  unrelated link dependency solely for a language mode would obscure intent.
  Prefer one top-level `CMAKE_CXX_STANDARD 20`,
  `CMAKE_CXX_STANDARD_REQUIRED ON`, and `CMAKE_CXX_EXTENSIONS OFF` contract.
- Add a small configure-time C++20 capability probe rather than inferring
  standard-library support from compiler identity alone. The probe should
  compile under the selected project mode and require `__cplusplus >= 202002L`,
  designated initialization, immediate (`consteval`) evaluation, and
  `<span>`/`std::span`. Report compiler ID/version and the missing capability
  in the failure diagnostic. Only clang 22.1.7 plus libc++ 220107 is evidenced
  by this audit; do not claim untested GCC, AppleClang, MSVC, or version floors
  solely from this host.
- Preserve the four manual `-std=gnu++20` language-under-test/workflow flags;
  they are outside the host-target migration.

## Proof

- Supervisor-owned `test_before.log` records the exact Step 1 baseline command:
  configure/build succeeded and full relevant CTest excluding
  `llvm_gcc_c_torture` ran 1312 tests with 1272 passed and 40 pre-existing
  failures. This audit packet did not rewrite the canonical log.
