Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

# Active Item

- Step 3: migrate the next aarch64 emitter helper cluster onto direct BIR
- Current slice: remove the remaining
  `parse_backend_minimal_folded_two_arg_direct_call_lir_module(...)` adapter by
  matching or collapsing that folded direct-call LIR shape directly in
  `src/backend/lowering/lir_to_bir.cpp`, then trim its legacy
  `call_decode.hpp` surface
- Next intended slice: decide whether the folded two-argument direct-call case
  should lower to direct BIR or collapse straight to a constant-return BIR
  module, then add the narrowest coverage needed before deleting the adapter

# Completed

- Removed the still-live
  `parse_backend_minimal_call_crossing_direct_call_lir_module(...)` adapter and
  its `ParsedBackendMinimalCallCrossingDirectCallLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by matching that call-crossing
  direct-call LIR shape directly in `src/backend/lowering/lir_to_bir.cpp`
- Added focused BIR lowering coverage in
  `tests/backend/backend_bir_lowering_tests.cpp` for the helper-first
  call-crossing direct-call module order so the direct matcher keeps
  validating that order-insensitive LIR slice after the adapter removal
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the call-crossing direct-call adapter removal
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the call-crossing direct-call adapter removal
- Rebuilt the full workspace with `cmake --build build -j8` successfully after
  the call-crossing direct-call adapter removal
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_before.log` / `test_after.log`; the workspace improved from
  `2675/2834` passing with 159 failures to `2834/2834` passing with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=159
  failed=-159` and zero newly failing tests

- Removed the still-live
  `parse_backend_minimal_dual_identity_direct_call_sub_lir_module(...)`
  adapter and its
  `ParsedBackendMinimalDualIdentityDirectCallSubLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by matching that dual-identity
  subtraction direct-call LIR shape directly in
  `src/backend/lowering/lir_to_bir.cpp`
- Added focused BIR lowering coverage in
  `tests/backend/backend_bir_lowering_tests.cpp` for the helper-first
  dual-identity subtraction direct-call module order so the direct matcher
  keeps validating that order-insensitive LIR slice after the adapter removal
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the dual-identity subtraction direct-call adapter removal
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the dual-identity subtraction direct-call adapter removal
- Rebuilt the full workspace with `cmake --build build -j8` successfully after
  the dual-identity subtraction direct-call adapter removal
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace stayed at
  `2834/2834` passing with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=13 failed=-13`
  and zero newly failing tests

- Removed the still-live
  `parse_backend_minimal_direct_call_identity_arg_lir_module(...)` adapter and
  its `ParsedBackendMinimalDirectCallIdentityArgLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by matching that identity direct-call
  LIR shape directly in `src/backend/lowering/lir_to_bir.cpp`
- Added focused BIR lowering coverage in
  `tests/backend/backend_bir_lowering_tests.cpp` for the helper-first identity
  direct-call module order so the direct matcher keeps validating that
  order-insensitive LIR slice after the adapter removal
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the identity direct-call adapter removal
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the identity direct-call adapter removal
- Rebuilt the full workspace with `cmake --build build -j8` successfully after
  the identity direct-call adapter removal
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace improved
  from `2821/2834` passing with 13 failures in `test_fail_before.log` to
  `2834/2834` passing with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=13 failed=-13`
  and zero newly failing tests

- Removed the still-live
  `parse_backend_minimal_direct_call_add_imm_lir_module(...)` adapter and its
  `ParsedBackendMinimalDirectCallAddImmLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by matching that add-immediate
  direct-call LIR shape directly in `src/backend/lowering/lir_to_bir.cpp`
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the add-immediate direct-call adapter removal
- Reran
  `ctest --test-dir build -R 'backend_bir_tests|backend_shared_util_tests' --output-on-failure`
  successfully after the add-immediate direct-call adapter removal
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_fail_after.log`; the workspace stayed at `2834/2834`
  passing with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=13 failed=-13`
  and zero newly failing tests

- Removed the still-live
  `parse_backend_minimal_two_arg_direct_call_lir_module(...)` adapter and its
  `ParsedBackendMinimalTwoArgDirectCallLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by matching that LIR shape directly in
  `src/backend/lowering/lir_to_bir.cpp`
- Added focused BIR lowering coverage in
  `tests/backend/backend_bir_lowering_tests.cpp` for the accepted one-local-slot
  two-argument direct-call rewrite form so the shared BIR lowering route keeps
  validating the non-trivial adapter behavior after removal
- Rebuilt `backend_bir_tests` successfully after the two-argument direct-call
  adapter removal
- Reran `ctest --test-dir build -R 'backend_bir_tests' --output-on-failure`
  successfully after the adapter removal
- Rebuilt the full workspace with `cmake --build build -j8` successfully after
  the adapter removal
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_fail_after.log`; the workspace improved from `2821/2834`
  passing with 13 failures in `test_fail_before.log` to `2834/2834` passing
  with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=13 failed=-13`
  and zero newly failing tests

- Removed the remaining
  `parse_backend_minimal_declared_direct_call_lir_module(...)` adapter and its
  `ParsedBackendMinimalDeclaredDirectCallLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by teaching
  `src/backend/lowering/lir_to_bir.cpp` to recognize and lower that LIR shape
  directly
- Rebuilt `backend_bir_tests` and `c4cll` successfully after the declared
  direct-call adapter removal
- Reran `ctest --test-dir build -R 'backend_bir_tests' --output-on-failure`
  successfully after the declared direct-call adapter removal
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace stayed at
  `2834/2834` passing with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=13 failed=-13`
  and zero newly failing tests

- Removed the still-live
  `parse_backend_minimal_void_direct_call_imm_return_lir_module(...)` adapter
  and its `ParsedBackendMinimalVoidDirectCallImmReturnLirModuleView` from
  `src/backend/lowering/call_decode.hpp` by lowering that LIR shape directly in
  `src/backend/lowering/lir_to_bir.cpp`
- Added focused BIR lowering coverage in
  `tests/backend/backend_bir_lowering_tests.cpp` for the minimal void
  direct-call plus fixed-return LIR slice so the shared BIR lowering route is
  validated without the removed adapter
- Rebuilt `backend_bir_tests` and `c4cll` successfully after the void
  direct-call adapter removal
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the adapter removal
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace is now at
  `2834/2834` passing with 0 failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=13 failed=-13`
  and zero newly failing tests

- Wired the native aarch64 direct-BIR emitter to the shared
  `parse_bir_minimal_dual_identity_direct_call_sub_module(...)` and
  `parse_bir_minimal_call_crossing_direct_call_module(...)` families in
  `src/backend/aarch64/codegen/emit.cpp`, removing the last bounded direct-call
  gap called out by the active Step 3 slice
- Extended `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` with focused
  direct-BIR and LIR-through-BIR coverage for the aarch64 dual-identity
  subtraction and call-crossing direct-call helper families
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the aarch64 direct-call emitter expansion
- Reran
  `ctest --test-dir build -R 'backend_bir_tests|backend_shared_util_tests' --output-on-failure`
  successfully after the aarch64 direct-call emitter expansion
- Reran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  refreshed `test_before.log` / `test_after.log`; the workspace improved from
  `2833/2834` passing with 1 failure (`c_testsuite_aarch64_backend_src_00121_c`)
  to `2834/2834` passing with 0 failures and zero newly failing tests

- Added native aarch64 direct-BIR emission for the minimal two-argument
  direct-call, direct-call-add-immediate, and identity direct-call families in
  `src/backend/aarch64/codegen/emit.cpp`
- Extended `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` with focused
  direct-BIR and LIR-through-BIR coverage for those three aarch64 helper
  families
- Rebuilt `backend_bir_tests` and `c4cll` successfully after the aarch64
  direct-call helper cluster landed
- Reran `ctest --test-dir build -R 'backend_bir_tests' --output-on-failure`
  successfully after the aarch64 direct-call helper cluster landed
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; compared with the
  existing `test_fail_before.log`, the workspace improved from `2821/2834`
  passing with 13 failures down to `2833/2834` passing with 1 failure, with
  zero newly failing tests

- Removed the remaining dead aarch64 emitter-local `BackendModule` parser
  inventory from `src/backend/aarch64/codegen/emit.cpp` for the
  conditional-return, countdown-loop, string-literal-char, scalar/external
  global load-store, and global-pointer-diff families after confirming the live
  aarch64 dispatcher only emits from `LirModule` and `bir::Module`
- Deleted the now-unused aarch64 legacy backend lookup helpers
  `find_global(...)` and backend `find_string_constant(...)` after the dead
  parser removal
- Rebuilt `c4cll`, `backend_bir_tests`, and `backend_shared_util_tests`
  successfully after the aarch64 dead parser cleanup
- Reran
  `ctest --test-dir build -R 'backend_bir_tests|backend_shared_util_tests' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_fail_after.log`; the workspace still has the same 13 known
  failures as `test_fail_before.log` with `2821/2834` tests passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests

- Removed the remaining dead x86 emitter-local `BackendModule` parser overloads
  for the conditional-return, countdown-loop, conditional-phi-join,
  local-array/global/string metadata cluster, and scalar-global-store-reload
  slices from `src/backend/x86/codegen/emit.cpp` after confirming the native
  x86 emitter only dispatches through LIR and direct-BIR entry points now
- Deleted the now-unused x86 legacy backend lookup helpers
  `find_single_zero_arg_i32_backend_definition(...)`, backend
  `find_global(...)`, and backend `find_string_constant(...)` after the dead
  `BackendModule` parser removal
- Rebuilt `c4cll`, `backend_bir_tests`, and `backend_shared_util_tests`
  successfully after the x86 dead parser cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace still has
  the same 13 known failures as `test_fail_before.log` with `2821/2834` tests
  passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Removed the dead x86 `BackendModule` forwarding wrappers for
  `emit_minimal_conditional_affine_i8_return_asm(...)` and
  `emit_minimal_conditional_affine_i32_return_asm(...)` now that all live
  dispatch sites already pass `target_triple` directly
- Removed the unused x86/aarch64 emitter-local `find_function(...)` helpers
  after confirming no remaining parser or emitter path reads backend function
  tables through those lookups
- Removed the dead x86
  `emit_function_prelude(..., const BackendModule&, ...)` overload after
  confirming every live call site already uses the symbol-only prelude helper
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the dead helper cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_fail_after.log`; the workspace still has the same 13 known
  failures as `test_fail_before.log` with `2821/2834` tests passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Removed the dead aarch64 `BackendModule` forwarding wrappers for
  `emit_minimal_conditional_affine_i8_return_asm(...)` and
  `emit_minimal_conditional_affine_i32_return_asm(...)` now that all live
  dispatch sites already pass `target_triple` directly
- Deleted the unused aarch64 legacy `BackendModule` parser/emitter cluster for
  `parse_minimal_conditional_phi_join_slice(...)`,
  `parse_minimal_local_array_slice(...)`,
  `emit_minimal_conditional_phi_join_asm(...)`, and
  `emit_minimal_local_array_asm(...)` after confirming no dispatch path still
  references that target-local fallback
- Rebuilt `c4cll`, `backend_bir_tests`, and `backend_shared_util_tests`
  successfully after the emitter-wrapper cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace still has
  the same 13 known failures as `test_fail_before.log` with `2821/2834` tests
  passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests

- Removed the dead x86/aarch64 `BackendModule` forwarding overloads for the
  scalar-global-load, scalar-global-store-reload, extern-scalar-global-load,
  extern-global-array-load, and global-pointer-diff emitter helpers now that
  all live dispatch sites already call the `target_triple` overloads directly
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the dead global-helper wrapper cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  recorded same-run before/after logs in `test_before.log` and `test_after.log`;
  both snapshots reported `2821/2834` passed with the same 13 known failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Removed the dead x86 emitter-local `BackendModule` forwarding overloads for
  `asm_symbol_name(...)` and `asm_private_data_label(...)` now that all live
  call sites already pass `target_triple` directly
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the x86 symbol/private-data wrapper cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  recorded same-run before/after logs in `test_before.log` and `test_after.log`;
  both snapshots reported `2821/2834` passed with the same 13 known failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Removed the dead x86 `BackendModule` forwarding wrappers for
  `emit_minimal_countdown_loop_asm(...)`,
  `emit_minimal_conditional_phi_join_asm(...)`, and
  `emit_minimal_local_array_asm(...)` now that all live call sites already
  pass `target_triple` directly
- Removed the dead aarch64 `BackendModule` forwarding wrapper for
  `emit_minimal_countdown_loop_asm(...)` now that all live call sites already
  pass `target_triple` directly
- Rebuilt `c4cll`, `backend_bir_tests`, and `backend_shared_util_tests`
  successfully after the dead emitter-wrapper cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace still has
  the same 13 known failures as `test_fail_before.log` with `2821/2834` tests
  passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Added missing aarch64 backend pipeline coverage for the Step 2 shared-BIR
  direct-call families so LIR helper/main, void-helper/fixed-return, and
  declared extern-call inputs are now all exercised end-to-end through
  `try_lower_to_bir(...)` on the native aarch64 path
- Rebuilt `backend_bir_tests` successfully after the aarch64 direct-call
  coverage expansion
- Reran `ctest --test-dir build -R 'backend_bir_tests' --output-on-failure`
  successfully after the aarch64 coverage expansion
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_fail_after.log`; the workspace still has the same 13 known
  failures as `test_fail_before.log` with `2821/2834` tests passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Removed the dead x86/aarch64 `BackendModule` single-function scalar-control-flow
  overload set for minimal return/add/sub/affine parsing and emission now that
  the active direct path already routes those slices through BIR-owned helpers
- Deleted the matching dead emitter-local `BackendModule` helper plumbing
  (`is_minimal_single_function_asm_slice(...)`,
  `minimal_single_backend_function(...)`, and
  `minimal_single_function_symbol(...)`) from the x86/aarch64 emitters where
  those helpers only served the removed legacy overloads
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the emitter overload cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace still has
  the same 13 known failures as `test_fail_before.log` with `2821/2834` tests
  passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests

- Switched the x86 zero-argument void-helper / fixed-immediate caller direct-BIR
  family to consume
  `parse_bir_minimal_void_direct_call_imm_return_module(...)` directly in
  `src/backend/x86/codegen/emit.cpp`, removing the target-local wrapper struct
  and parser that only forwarded helper/caller names plus the fixed return
  immediate
- Tightened the focused x86 BIR pipeline regression for that family so the
  direct-BIR test still passes when the caller appears before the helper in the
  module function list, proving the shared BIR parser is the real pairing seam
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the x86 void-direct-call cleanup
- Reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  refreshed `test_after.log` / `test_fail_after.log`; the workspace still has
  the same 13 known failures as `test_fail_before.log` with `2821/2834` tests
  passing
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Removed the next verified-dead target-local `BackendModule` forwarding
  overloads from `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp` after confirming all live callers
  already pass target triples or BIR-owned slices directly for the
  conditional-return, string-literal-char, and call-crossing direct-call helper
  families
- Rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`
  successfully after the emitter-local wrapper cleanup
- Reran
  `ctest --test-dir build -R 'backend_(shared_util_tests|bir_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite twice
  to establish same-tree before/after logs for this maintenance slice; both
  snapshots reported `2821/2834` passed with the same 13 known failures
- Ran the c4c regression guard script with
  `--allow-non-decreasing-passed`; it passed with `delta: passed=0 failed=0`
  and zero newly failing tests
- Extended the focused x86 BIR pipeline regression so folded-two-arg,
  dual-identity subtraction, and call-crossing direct-call LIR inputs are all
  required to lower into shared BIR-owned shapes before target emission
- Removed the dead shared
  `ParsedBackendMinimalDirectCallLirModuleView` /
  `parse_backend_minimal_direct_call_lir_module(...)` adapter from
  `src/backend/lowering/call_decode.hpp` now that the zero-arg direct-call LIR
  family already lowers through `try_lower_to_bir(...)` and no live target
  still consumes the old parser view
- Rebuilt `backend_shared_util_tests`, `backend_bir_tests`, and `c4cll`
  successfully after the dead shared LIR-parser cleanup
- Reran
  `ctest --test-dir build -R 'backend_(shared_util_tests|bir_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  recorded the current workspace baseline in `test_after.log`: `2674/2834`
  tests passed, with 160 failures in this snapshot (`test_before.log` was not
  present at the start of the run, so no same-run monotonic diff was possible)
- Removed the dead x86-only folded-two-arg return-immediate parser wrapper and
  the dead x86-only dual-identity direct-call subtraction LIR slice adapter
  from `src/backend/x86/codegen/emit.cpp` now that `try_emit_direct_lir_module`
  lowers those families through `try_lower_to_bir(...)` before native emission
- Rebuilt `backend_bir_tests` and reran
  `ctest --test-dir build -R 'backend_bir_tests' --output-on-failure`
  successfully
- Reran the full `ctest --test-dir build -j --output-on-failure` suite; the
  workspace still has the same 13 known dirty-baseline failures in
  `test_after.log` and `backend_bir_tests` still passes
- Read `plan.md` and the linked source idea
- Confirmed `todo.md` was missing and initialized execution state for this run
- Inspected `call_decode.hpp` and found existing BIR helpers for minimal direct
  call and declared direct call module parsing
- Recorded the full-suite baseline in `test_before.log`
- Fixed `parse_bir_minimal_direct_call_module(...)` so it matches a helper
  definition returning an immediate instead of a declaration-only placeholder
- Extended `parse_bir_minimal_declared_direct_call_module(...)` to preserve
  whether the caller returns the call result or a fixed immediate
- Added focused shared-util coverage for both BIR direct-call parser helpers
- Rebuilt from scratch and confirmed full-suite parity:
  `2834/2834` tests passed before and after
- Added focused x86 BIR pipeline coverage for a direct BIR helper/main
  zero-argument direct-call module
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_direct_call_module(...)` before the affine-return BIR
  subset so direct BIR helper/main call pairs no longer require a legacy
  `BackendModule` parser on that path
- Rebuilt `backend_bir_tests` and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully
- Switched `src/backend/aarch64/codegen/emit.cpp` to consume
  `parse_bir_minimal_direct_call_module(...)` before the affine-return BIR
  subset so direct BIR helper/main call pairs now avoid the legacy
  `BackendModule` route on the native aarch64 entry too
- Added focused aarch64 BIR pipeline coverage for a direct BIR helper/main
  zero-argument direct-call module
- Rebuilt `backend_bir_tests` and confirmed full-suite parity again:
  `2834/2834` tests passed in `test_after.log`
- Extended `parse_bir_minimal_declared_direct_call_module(...)` to decode
  bounded BIR extern-call arguments into concrete immediate vs pointer-style
  metadata using declaration param types plus module symbol tables
- Added shared-util coverage proving the BIR declared-direct-call parser
  preserves pointer-style string-constant operands as concrete extern-call args
- Switched the native aarch64 direct-BIR emitter to consume the new BIR
  declared-direct-call view so that declared extern-call modules no longer need
  the legacy `BackendModule` parser on that path
- Added focused aarch64 direct-BIR pipeline coverage for a declared direct-call
  module that passes a string-constant pointer to an extern declaration and
  then returns a fixed immediate
- Rebuilt the affected backend test binaries and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully
- Confirmed full-suite parity for this slice: `2834/2834` tests passed in
  `test_after.log`
- Added focused x86 direct-BIR pipeline coverage for a declared direct-call
  module that passes a string-constant pointer to an extern declaration and
  then returns a fixed immediate
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_declared_direct_call_module(...)` so direct BIR declared
  extern-call modules no longer require the legacy `BackendModule` parser on
  the native x86 entry
- Rebuilt `backend_bir_tests` and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully
- Confirmed full-suite parity for the x86 declared-direct-call slice:
  `2834/2834` tests passed in `test_after.log`
- Removed the now-dead
  `ParsedBackendMinimalDeclaredDirectCallModuleView` /
  `parse_backend_minimal_declared_direct_call_module(...)` family from
  `src/backend/lowering/call_decode.hpp` once both native emitters had already
  switched to the BIR declared-direct-call view
- Deleted the matching dead `BackendModule`-only declared-direct-call emitter
  overloads from `src/backend/x86/codegen/emit.cpp` and
  `src/backend/aarch64/codegen/emit.cpp`
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, then reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully
- Reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully and confirmed parity again: `2834/2834` tests passed in
  `test_after.log`
- Added a bounded `try_lower_to_bir(...)` path for the remaining minimal LIR
  declared-direct-call module shape, including string-pool decoding into BIR
  string constants and synthesized BIR declaration/main functions
- Deleted the redundant x86
  `parse_backend_minimal_declared_direct_call_lir_module(...)` dispatch and
  private LIR-specific emitter overload from `src/backend/x86/codegen/emit.cpp`
- Added focused BIR lowering coverage for the minimal LIR
  declared-direct-call slice plus an x86 end-to-end pipeline test proving LIR
  input now stays on the BIR-owned extern-call route
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully with `2834/2834` tests passing in `test_after.log`
- Reran the full `ctest --test-dir build -j --output-on-failure` suite after
  the dual-identity subtraction migration and confirmed parity again:
  `2834/2834` tests passed in `test_after.log`
- Added focused shared-util coverage for the new BIR dual-identity direct-call
  subtraction parser view
- Added a bounded `try_lower_to_bir(...)` path for the dual-identity
  subtraction LIR family so it synthesizes shared BIR identity helpers and a
  BIR subtraction caller instead of depending on x86's private direct LIR
  emitter seam
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_dual_identity_direct_call_sub_module(...)` view for
  direct BIR emission and removed the earlier x86-only direct-LIR fast path
  for that family
- Added focused BIR lowering coverage plus direct-BIR and LIR-through-BIR x86
  pipeline coverage for the dual-identity direct-call subtraction family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, then reran the
  focused `dual_identity` cases in both test binaries successfully
- Added a shared BIR parser for the minimal single-argument identity-return
  direct-call helper/main module shape
- Added a bounded `try_lower_to_bir(...)` path for the matching minimal LIR
  identity-return direct-call family so it synthesizes a shared BIR helper and
  caller instead of depending on the x86-only direct LIR emitter seam
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_direct_call_identity_arg_module(...)` view for direct BIR
  emission and the shared identity-return slice abstraction for LIR input
- Added focused shared-util coverage, BIR lowering coverage, and x86
  end-to-end pipeline coverage for the identity-return direct-call family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran both test
  binaries directly, and reran the full `ctest --test-dir build -j
  --output-on-failure` suite successfully with `2834/2834` tests passing in
  `test_after.log`
- Added a bounded `try_lower_to_bir(...)` path for the folded two-argument
  direct-call LIR family so it collapses the helper/main slice to a shared BIR
  caller return-immediate shape instead of depending on x86's private legacy
  parser dispatch
- Removed the now-dead x86-only folded two-argument direct-call direct-emission
  dispatch from `src/backend/x86/codegen/emit.cpp`
- Added focused BIR lowering coverage for the folded two-argument direct-call
  LIR slice plus an x86 end-to-end pipeline test proving the LIR route now
  stays on the shared BIR-owned emitter path
- Rebuilt `backend_bir_tests`, reran the binary directly, and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully with
  `2834/2834` tests passing in `test_after.log`
- Added an order-independent `try_lower_to_bir(...)` path for the zero-arg
  minimal LIR direct-call helper/main family so it now synthesizes the shared
  two-function BIR direct-call shape instead of depending on x86's private
  legacy parser dispatch
- Removed the now-dead x86-only
  `parse_backend_minimal_direct_call_lir_module(...)` direct-emission dispatch
  and matching private emitter overload from `src/backend/x86/codegen/emit.cpp`
- Added focused BIR lowering coverage for the zero-arg minimal direct-call LIR
  slice plus an x86 end-to-end pipeline test proving the LIR route now stays
  on the shared BIR direct-call emitter path
- Rebuilt `backend_bir_tests`, reran the binary directly, and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully with
  `2834/2834` tests passing in `test_after.log`
- Added focused shared-util coverage for the new BIR
  two-argument direct-call parser view, plus BIR lowering coverage for the
  matching minimal LIR helper/main slice
- Added focused x86 BIR pipeline coverage for both direct-BIR input and
  LIR-through-BIR input for the minimal two-argument direct-call family
- Added a bounded `try_lower_to_bir(...)` path for the minimal two-argument
  direct-call LIR helper/main shape so it synthesizes a shared BIR helper and
  caller instead of relying on the x86-only direct LIR fast path
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_two_arg_direct_call_module(...)` view for direct BIR
  emission and removed the earlier x86-only direct-LIR dispatch for that
  family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully with `2834/2834` tests passing in `test_after.log`
- Added a shared BIR parser for the minimal single-argument add-immediate
  direct-call helper/main module shape
- Added a bounded `try_lower_to_bir(...)` path for the matching minimal LIR
  add-immediate direct-call family so it synthesizes a shared BIR helper and
  caller instead of depending on x86-only direct LIR emission
- Switched `src/backend/x86/codegen/emit.cpp` to consume the new
  `parse_bir_minimal_direct_call_add_imm_module(...)` view for direct BIR
  emission and the shared add-immediate slice abstraction for LIR input
- Added focused shared-util coverage, BIR lowering coverage, and x86
  end-to-end pipeline coverage for the add-immediate direct-call family
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully with `2834/2834` tests passing in `test_after.log`
- Added a shared BIR parser for the minimal call-crossing direct-call
  helper/main family, preserving the helper identity, recovered source add
  value name, source immediate, and helper add immediate
- Added a bounded `try_lower_to_bir(...)` path for the minimal call-crossing
  LIR family so x86 call-crossing input now routes through shared BIR instead
  of depending on the direct-LIR emission shortcut
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_call_crossing_direct_call_module(...)` for direct BIR
  emission and removed the earlier direct-LIR call-crossing fast path from
  `try_emit_direct_lir(...)`
- Added focused shared-util coverage, BIR lowering coverage, and x86 pipeline
  coverage for both direct-BIR and LIR-through-BIR minimal call-crossing
  direct-call modules
- Rebuilt `backend_bir_tests` and `backend_shared_util_tests`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  against the repo's current dirty baseline with the same 13 known failures in
  `test_before.log` and `test_after.log`
- Tightened the shared BIR call-crossing parser test so it also proves the
  recovered regalloc source value stays aligned with the source add result, the
  call operand, and the final add lhs on the BIR-owned path
- Removed the dead
  `ParsedBackendMinimalCallCrossingDirectCallModuleView` /
  `parse_backend_minimal_call_crossing_direct_call_module(...)` family from
  `src/backend/lowering/call_decode.hpp` now that the x86 call-crossing slice
  only uses the shared BIR view
- Deleted the matching dead x86 `BackendModule` call-crossing slice wrapper and
  the dead aarch64 `BackendModule` call-crossing emitter overload that still
  referenced the removed legacy parsed view
- Rebuilt `backend_shared_util_tests` and `backend_bir_tests`, reran both test
  binaries directly, and reran the full `ctest --test-dir build -j
  --output-on-failure` suite successfully against the repo's current known
  dirty baseline with the same 13 failures recorded in `test_after.log`
- Identified the zero-argument void-helper / fixed-immediate caller family as
  the next smallest remaining rich direct-call seam that could move to shared
  BIR without widening globals, local-slot, or extern metadata contracts
- Added `ParsedBirMinimalVoidDirectCallImmReturnModuleView` /
  `parse_bir_minimal_void_direct_call_imm_return_module(...)` so shared
  call-decode now owns the BIR helper/main pairing for that void direct-call
  family
- Added a bounded `try_lower_to_bir(...)` path for the matching minimal LIR
  void direct-call family so x86 LIR input now lowers through shared BIR
  instead of depending on the direct LIR-only emitter shortcut
- Switched x86 direct BIR emission to recognize the new BIR void direct-call
  slice and removed the dead x86 LIR-only direct-emission seam for that family
- Switched native aarch64 direct BIR emission to recognize the new shared BIR
  void direct-call slice and deleted the dead legacy `BackendModule` emitter
  overload for that family
- Added focused shared-util coverage plus x86/aarch64 direct-BIR pipeline
  coverage and x86 LIR-through-BIR pipeline coverage for the void direct-call
  family
- Rebuilt `backend_shared_util_tests`, `backend_bir_tests`, and `c4cll`, reran
  `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`,
  and reran the full `ctest --test-dir build -j --output-on-failure` suite
  successfully against the repo's current dirty baseline with the same 13
  failures recorded in `test_before.log` and `test_after.log`
- Deleted the dead folded two-argument direct-call legacy seam that no longer
  had live callers after the route already collapsed to shared BIR lowering
  plus native return-immediate emission:
  `ParsedBackendMinimalFoldedTwoArgDirectCallModuleView`,
  `parse_backend_minimal_folded_two_arg_direct_call_module(...)`, the unused
  x86 folded-two-arg parse helpers, and the dead aarch64 folded-two-arg legacy
  emitter overload
- Rebuilt `backend_shared_util_tests`, `backend_bir_tests`, and `c4cll`
  successfully after the folded-two-arg seam cleanup
- Reran `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite after
  the cleanup; the workspace still has the same 13 known failures listed in
  `test_after.log`
- Added a focused x86 regression proving the two-arg, add-immediate, and
  identity-return direct-call LIR helper families already lower into the
  shared BIR parser views before target emission
- Removed the redundant x86 direct-LIR parser/emitter shortcuts and
  `try_emit_direct_lir(...)` dispatch for those same two-arg, add-immediate,
  and identity-return direct-call helper families now that the BIR-owned route
  already covers them
- Rebuilt `backend_bir_tests` and reran
  `ctest --test-dir build -R 'backend_bir_tests' --output-on-failure`
  successfully after the shortcut cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite after
  the cleanup; the workspace still has the same 13 known failures in
  `test_after.log` (`test_before.log` is not present in this workspace
  snapshot)
- Removed the dead `ParsedBackendMinimalStructuredDirectCallModuleView` /
  `parse_backend_minimal_*direct_call*_module(...)` `BackendModule` parser
  family from `src/backend/lowering/call_decode.hpp` once the zero-arg,
  single-arg, two-arg, and dual-identity direct-call slices no longer had live
  `BackendModule` consumers
- Deleted the matching dead x86 and aarch64 `BackendModule` direct-call emitter
  overloads, plus the dead x86 `BackendModule` dual-identity slice wrapper
- Rebuilt `backend_shared_util_tests`, `backend_bir_tests`, and `c4cll`
  successfully after the dead-seam cleanup
- Reran `ctest --test-dir build -R 'backend_(bir_tests|shared_util_tests)' --output-on-failure`
  successfully after the cleanup
- Reran the full `ctest --test-dir build -j --output-on-failure` suite and
  recorded the current dirty baseline in `test_after.log`: `2821/2834` tests
  passed, with the same 13 known failures already tracked in `todo.md`

# Next

- After validation, inventory the remaining genuinely live
  `BackendModule`-only helper families; the next likely work is no longer in
  the pure helper/main direct-call slices, but in richer mixed-metadata seams
- Continue shrinking stale direct-call helper families one seam at a time now
  that the zero-arg, declared-direct-call, minimal two-argument helper/main,
  single-argument add-immediate, single-argument identity-return, folded
  two-argument helper/main, dual-identity subtraction, and call-crossing paths
  no longer need private x86 LIR dispatch
- Use the next slice to make `lir_to_backend_ir.*` more obviously adapter-only
  before Step 4 collapse work starts

# Notes

- Updated the stale `call_decode.hpp` comment to reflect that BIR already owns
  part of the direct-call surface, while legacy helpers still carry richer
  typed-call / extern-arg detail
- `test_before.log` was not present in the workspace at the start of this
  iteration, so this run used focused rebuilds plus a fresh full-suite
  `test_after.log` check instead of a same-run before/after diff
- This slice removes one more dead direct-call compatibility seam from shared
  call-decode itself: the unused zero-arg direct-call LIR parser/view is gone,
  while the live zero-arg LIR-to-BIR lowering helpers remain in
  `src/backend/lowering/lir_to_bir.cpp`
- The current workspace full-suite baseline is much dirtier than the older
  `todo.md` notes from prior iterations: this run recorded `2674/2834` passing
  with 160 failures in `test_after.log`
- This run's full-suite check finished with the same 13 known failures listed
  in `test_after.log`:
  `backend_runtime_local_arg_call`, `backend_runtime_param_slot`,
  `backend_runtime_two_arg_both_local_arg`,
  `backend_runtime_two_arg_both_local_double_rewrite`,
  `backend_runtime_two_arg_both_local_first_rewrite`,
  `backend_runtime_two_arg_both_local_second_rewrite`,
  `backend_runtime_two_arg_first_local_rewrite`,
  `backend_runtime_two_arg_helper`, `backend_runtime_two_arg_local_arg`,
  `backend_runtime_two_arg_second_local_arg`,
  `backend_runtime_two_arg_second_local_rewrite`,
  `c_testsuite_aarch64_backend_src_00116_c`, and
  `c_testsuite_aarch64_backend_src_00121_c`
- This slice removes the last call-crossing-specific `BackendModule` parse
  helper from the shared call-decode surface; the remaining legacy direct-call
  seams are the richer typed/direct-call families that still depend on
  `BackendModule` shape not yet modeled in shared BIR helpers
  `BackendModule`; the shared parser/emitter route is BIR-owned, and the x86
  LIR-only compatibility seam has also been deleted
- `test_before.log` was not present in the workspace at the start of this run;
  this slice rebuilt the touched backend targets, reran targeted backend
  coverage, and then recorded full-suite success in `test_after.log` at
  `2834/2834`
- This slice removes the x86 emitter's private zero-arg direct-call LIR
  dispatch, but the shared LIR shape recognizer still exists as lowering glue
  for `try_lower_to_bir(...)`
- `test_before.log` was not present in the workspace at the start of this
  slice; the rerun full-suite result in `test_after.log` is `2834/2834`
- This slice removes the x86 emitter's private minimal two-argument direct-call
  LIR dispatch, while keeping the existing LIR recognizer as bounded lowering
  glue for `try_lower_to_bir(...)`
- This slice removes another x86-only direct-call seam from the legacy route:
  the minimal single-argument add-immediate helper/main family now lowers
  through shared BIR and has a direct-BIR emitter path on x86
- This slice removes another x86-only direct-call seam from the legacy route:
  the minimal single-argument identity-return helper/main family now lowers
  through shared BIR and has a direct-BIR emitter path on x86
- This slice removes the x86 emitter's private folded two-argument direct-call
  LIR dispatch; the helper/main family now lowers through shared BIR as a
  collapsed caller return-immediate shape
- This slice removes the x86 emitter's private dual-identity subtraction
  direct-call LIR fast path; the helper/main family now lowers through shared
  BIR and has a direct-BIR emitter path on x86
- `test_before.log` and `test_after.log` both recorded the same current
  full-suite baseline for this run: `99%` passed with 13 known failures
  (`backend_runtime_*` plus two aarch64 `c_testsuite` cases), so this slice
  was monotonic against the repo's dirty state
