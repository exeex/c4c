# AArch64 Backend Port Todo

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Source Plan: plan.md

## Current Active Item

- Step 3: port the first integer/control-flow slice
- Iteration slice: inspect the next narrow AArch64 variadic follow-on after landing target-local scalar `va_arg` lowering for integer loads from a local `va_list`

## Next Intended Slice

- inspect the next target-local AArch64 variadic lowering gap after integer scalar `va_arg`, most likely the first floating-point scalar `va_arg` path that reuses the frontend's `vaarg.fp.*` helper blocks
- compare the resulting LLVM IR shape against Clang on `aarch64-unknown-linux-gnu` before widening beyond one floating-point `va_arg` case
- keep the next slice focused on one target-local helper boundary or one missing runtime-backed backend capability
- avoid broadening beyond the active AArch64 Step 3 runbook without recording a separate idea

## Completed Items

- activated `ideas/open/02_backend_aarch64_port_plan.md` into `plan.md`
- initialized execution-state tracking for the active AArch64 runbook
- completed Step 1 inspection of the ref AArch64 backend surfaces
- mapped the first ref port slice onto `src/backend/aarch64/` plus explicit factory wiring in `src/backend/backend.cpp`
- landed the Step 2 scaffold in `src/backend/aarch64/emitter.*`
- added target-local scaffold coverage in `tests/backend/backend_lir_adapter_tests.cpp`
- split the AArch64 scaffold into target-local `frame`, `alu`, and `branch` helpers under `src/backend/aarch64/`
- replaced the scaffold delegation with target-local LLVM-text emission for `ret`, integer `add`, integer `icmp`, `zext`, `br`, and `condbr`
- added compare/branch unit coverage in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/branch_if_lt.c` to prove multi-block compare/branch lowering through the external AArch64 toolchain path
- added target-local direct-call rendering in `src/backend/aarch64/calls.*`
- added unit coverage for AArch64 direct helper calls in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/call_helper.c` to prove helper-call lowering through the external AArch64 toolchain path
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=488/502 -> 489/503` and zero newly failing tests
- allowed the AArch64 emitter to preserve module `type_decls` needed by simple function lowering
- added target-local AArch64 `alloca`/`load`/`store` rendering in `src/backend/aarch64/memory.*`
- added unit coverage for local temporary stack-slot emission in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/local_temp.c` for local integer temporary runtime coverage on supported AArch64 hosts
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=489/503 -> 491/504` and zero newly failing test names; one prior cpp EASTL failure dropped from the failing set
- added unit coverage for AArch64 modified-parameter slot emission plus typed direct helper calls in `tests/backend/backend_lir_adapter_tests.cpp`
- allowed the AArch64 call renderer to preserve typed direct-call suffixes while still rejecting indirect typed calls
- added `tests/c/internal/backend_case/param_slot.c` for modified-parameter runtime coverage on supported AArch64 hosts
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=490/504 -> 491/505` and zero newly failing tests
- added target-local AArch64 `getelementptr` rendering in `src/backend/aarch64/memory.cpp`
- extended the target-local AArch64 cast slice to render `sext` needed by local indexed addressing
- added unit coverage for AArch64 local array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/local_array.c` for local array runtime coverage on supported AArch64 hosts
- rebuilt from a clean `build/` directory, recorded `test_after.log`, and passed the monotonic regression guard against `test_before.log` with `passed=491/505 -> 493/506` and zero newly failing tests
- added unit coverage for AArch64 struct-by-value parameter member-array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/param_member_array.c` for struct-by-value member-array runtime coverage on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_param_member_array` pass for the new parameter/member stack-addressing slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite after landing the new coverage; the new backend test passed, while the suite still reports unrelated existing failures outside this AArch64 slice
- added unit coverage for nested by-value member-array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/nested_param_member_array.c` for nested by-value member-array runtime coverage on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_nested_param_member_array` pass for the new nested by-value stack-addressing slice
- rebuilt from a clean `build/` directory, recorded `test_fail_after.log`, and passed the monotonic regression guard against `test_fail_before.log` with `passed=495/508 -> 496/509` and zero newly failing tests
- added unit coverage for AArch64 `ret void` rendering in `tests/backend/backend_lir_adapter_tests.cpp`
- extracted target-local AArch64 return rendering from `src/backend/aarch64/frame.cpp` into `src/backend/aarch64/returns.*`
- rebuilt from a clean `build/` directory, recorded `test_fail_after.log`, and passed the maintenance regression guard against `test_fail_before.log` with `passed=496/509 -> 496/509` and zero newly failing tests
- added unit coverage for AArch64 module header and declaration rendering in `tests/backend/backend_lir_adapter_tests.cpp`
- extracted target-local AArch64 module validation and function/module prologue helpers from `src/backend/aarch64/frame.*` into `src/backend/aarch64/prologue.*`
- reran `backend_lir_adapter_tests` and the full `ctest --test-dir build -j --output-on-failure` suite, then passed the maintenance regression guard against `test_fail_before.log` with `passed=496/509 -> 496/509` and zero newly failing tests
- added target-local AArch64 module global-definition rendering in `src/backend/aarch64/globals.*`
- added unit coverage for AArch64 global-definition plus global-load emission in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/global_load.c` for runtime coverage of simple global loads on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_global_load` pass for the new global-definition slice
- rebuilt from a clean `build/` directory, recorded `test_fail_after.log`, and reproduced one new unrelated full-suite failure outside this AArch64 slice (`cpp_positive_sema_eastl_inherited_trait_value_template_arg_parse_cpp`) while the new targeted backend coverage stayed green
- added target-local AArch64 module string-pool rendering in `src/backend/aarch64/globals.*`
- added unit coverage for AArch64 string-pool plus string-byte load emission in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/string_literal_char.c` for runtime coverage of string-literal byte loads on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_string_literal_char` pass for the new string-pool slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=496/510 -> 498/511` and zero newly failing tests; one unrelated prior C++ failure dropped from the failing set
- added target-local AArch64 module extern-declaration rendering in `src/backend/aarch64/globals.*`
- removed the AArch64 module-level validation rejection for `extern_decls` in `src/backend/aarch64/prologue.cpp`
- added unit coverage for AArch64 extern-declared direct calls in `tests/backend/backend_lir_adapter_tests.cpp`
- verified `backend_lir_adapter_tests` and the existing targeted AArch64 backend runtime tests pass after landing the extern-declaration slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=496/510 -> 498/511` and zero newly failing tests; one unrelated prior C++ failure dropped from the failing set
- confirmed unresolved `extern` globals lower to `external global` declarations plus scalar and array-addressing loads under `--codegen lir --target aarch64-unknown-linux-gnu`, matching Clang's declaration shape for the same probes
- added unit coverage for AArch64 extern-global scalar loads and extern-global array decay/indexed addressing in `tests/backend/backend_lir_adapter_tests.cpp`
- verified `backend_lir_adapter_tests` passes after landing the extern-global usage coverage slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=496/510 -> 498/511` and zero newly failing tests; one unrelated prior C++ failure dropped from the failing set (`cpp_positive_sema_eastl_inherited_trait_value_template_arg_parse_cpp`)
- added unit coverage for AArch64 scalar `bitcast` rendering in `tests/backend/backend_lir_adapter_tests.cpp`
- extended the target-local AArch64 cast slice to render `bitcast` in `src/backend/aarch64/alu.cpp`
- rebuilt and reran `backend_lir_adapter_tests`, then passed the maintenance regression guard against `test_fail_before.log` with `passed=527/534 -> 527/534`, zero newly failing tests, and the same seven unrelated full-suite failures before and after
- added target-local AArch64 `ptrtoint` rendering in `src/backend/aarch64/alu.cpp` for byte-granular pointer-difference lowering
- added unit coverage for AArch64 global byte-array pointer-difference lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/global_char_pointer_diff.c` for runtime coverage of byte-granular global symbol-address subtraction on supported AArch64 hosts
- updated `tests/c/internal/InternalTests.cmake` with the expected runtime exit code for the new backend case
- verified `backend_lir_adapter_tests` and `backend_runtime_global_char_pointer_diff` pass for the new global pointer-difference slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=496/510 -> 498/512` and zero newly failing tests
- added target-local AArch64 `sdiv` rendering in `src/backend/aarch64/alu.cpp` for scaled non-byte pointer-difference lowering
- added unit coverage for AArch64 global `int*` pointer-difference lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/global_int_pointer_diff.c` for runtime coverage of scaled global symbol-address subtraction on supported AArch64 hosts
- updated `tests/c/internal/InternalTests.cmake` with the expected runtime exit code for the new backend case
- verified `backend_lir_adapter_tests` and `backend_runtime_global_int_pointer_diff` pass for the new scaled pointer-difference slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=496/510 -> 501/513` and zero newly failing tests
- added target-local AArch64 `inttoptr` rendering in `src/backend/aarch64/alu.cpp` for global address round-tripping
- added unit coverage for AArch64 global pointer `ptrtoint`/`inttoptr` round-tripping in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/global_int_pointer_roundtrip.c` for runtime coverage of global address round-tripping on supported AArch64 hosts
- updated `tests/c/internal/InternalTests.cmake` with the expected runtime exit code for the new backend case
- verified `backend_lir_adapter_tests` and `backend_runtime_global_int_pointer_roundtrip` pass for the new round-trip cast slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=496/510 -> 501/514` and zero newly failing tests; one unrelated prior backend failure dropped from the failing set (`backend_lir_missing_toolchain_diagnostic`)
- added target-local AArch64 intrinsic declarations plus `llvm.va_start`/`llvm.va_end`/`llvm.va_copy` call rendering in `src/backend/aarch64/globals.*` and `src/backend/aarch64/memory.cpp`
- added unit coverage for AArch64 variadic intrinsic declaration/call rendering in `tests/backend/backend_lir_adapter_tests.cpp`
- verified `backend_lir_adapter_tests` passes for the new variadic intrinsic slice
- rebuilt from a clean `build/` directory, recorded `test_fail_after.log`, and passed the maintenance regression guard against `test_fail_before.log` with `passed=527/534 -> 527/534`, zero newly failing tests, and the same seven unrelated full-suite failures before and after
- added target-local AArch64 scalar `va_arg` rendering in `src/backend/aarch64/memory.cpp` plus `phi` join rendering in `src/backend/aarch64/branch.cpp` for the frontend's target-local variadic helper CFG
- added unit coverage for AArch64 scalar `va_arg` plus pointer `phi` joins in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/variadic_sum2.c` for runtime coverage of integer scalar `va_arg` on supported AArch64 hosts
- updated `tests/c/internal/InternalTests.cmake` with the expected runtime exit code for the new backend case
- verified the AArch64 scalar `va_arg` probe matches Clang's `aarch64-unknown-linux-gnu` control-flow shape through the register/stack split and `phi` join
- verified `backend_lir_adapter_tests` and `backend_runtime_variadic_sum2` pass for the new scalar variadic slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite, then passed the regression guard against `test_fail_before.log` with `passed=527/534 -> 530/535`, zero newly failing tests, and two prior compare-smoke failures dropping from the failing set (`compare_smoke_scalar`, `compare_smoke_struct`)

## Blockers

- full-suite `ctest` is not clean yet due unrelated existing failures outside the active AArch64 slice (`positive_sema_ok_fn_returns_variadic_fn_ptr_c`, `compare_smoke_switch`, and several longstanding C++ positive-case failures)

## Resume Notes

- do not reopen the closed LIR adapter idea; build on the existing backend boundary
- keep the port mechanically close to ref before introducing C++-specific cleanup
- preserve the external toolchain fallback contract already established for backend runtime tests
- the scaffold should stay target-local: x86_64/i686/riscv64 keep the existing generic backend entry until their own plans are active
- the current AArch64 branch slice still relies on the frontend's existing LLVM-text LIR conventions; direct calls and stack slots are the next boundary to port
- direct calls now render through the target-local AArch64 emitter for simple direct `@helper(...)` cases; indirect calls and typed callee suffixes remain out of slice
- simple local integer temporaries now emit through the target-local AArch64 memory helpers; broader slot flows and runtime execution still depend on supported AArch64 hosts/toolchains for end-to-end coverage
- modified-parameter slots now execute through the target-local AArch64 path, including typed direct helper calls with arguments
- local array stack-slot addressing now executes through target-local AArch64 `getelementptr` plus index-widening casts on supported hosts
- struct-by-value parameter member-array addressing is now covered in both backend emitter and runtime tests on the AArch64 path
- isolated AArch64 probes show nested `->` member-array addressing already lowers and executes through the current target-local memory helpers; this iteration is codifying that slice in repo tests
- added unit coverage for nested `->` member-array GEP lowering in `tests/backend/backend_lir_adapter_tests.cpp`
- added `tests/c/internal/backend_case/nested_member_pointer_array.c` for nested member-pointer runtime coverage on supported AArch64 hosts
- verified `backend_lir_adapter_tests` and `backend_runtime_nested_member_pointer_array` pass for the new nested member-pointer stack-addressing slice
- reran the full `ctest --test-dir build -j --output-on-failure` suite after landing the new coverage; the regression guard passed with `passed=494/507 -> 495/508`, zero newly failing tests, and the new backend runtime test accounted for the pass-count increase
- nested by-value aggregate member-array addressing also lowers through the current target-local AArch64 memory helpers; this iteration codifies that slice in repo tests
- module validation plus function/module prologue rendering are now isolated in `src/backend/aarch64/prologue.*`; `src/backend/aarch64/frame.*` is down to entry allocas plus function epilogue helpers
- plain module global definitions now render through `src/backend/aarch64/globals.*`; `string_pool` and `extern_decls` remain out of slice and are the most obvious next global-addressing boundaries
- module string-pool definitions now also render through `src/backend/aarch64/globals.*`; `extern_decls` remains the next most obvious module-level global-addressing boundary
- module extern declarations now also render through `src/backend/aarch64/globals.*`; the next obvious global-addressing follow-on is explicit external-global usage or other target-local symbol-address helpers aligned with ref `codegen/globals.rs`
- byte-granular global pointer subtraction now executes through the target-local AArch64 path via `ptrtoint`; wider pointer-difference follow-ons may still require extra cast or division support
- non-byte global pointer subtraction now also executes through the target-local AArch64 path via `ptrtoint` plus `sdiv`
- global address round-tripping now also executes through the target-local AArch64 path via `ptrtoint` plus `inttoptr`
- minimal AArch64 variadic probes now also get through integer scalar `va_arg` via the frontend's helper-generated register/stack split plus `phi` join; the next likely blocker is the first floating-point `va_arg` helper path rather than integer scalar variadics
