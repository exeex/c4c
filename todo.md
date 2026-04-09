# Active Todo

Status: Active
Source Idea: ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md
Source Plan: plan.md

## Active Item

- Step 5: pull phi and CFG normalization behind the BIR boundary
- Current slice: continue the Step 5 AArch64 generic-emitter CFG audit after
  widening the synthetic alloca-backed `switch` fence beyond the hard-coded
  two-case shape and re-evaluate whether the remaining constant-selector or
  branch-only multi-block direct-LIR seams can be fenced without cutting into
  still-broad production fallback coverage or spilling into Step 6 MIR-owned
  liveness/regalloc work
- Next intended slice: inspect whether synthetic constant-selector single-case
  `switch` return modules still succeed after `try_lower_to_bir(...)` declines
  them and decide whether the next safe move is a matching narrow generic-emitter
  prune or a shared BIR lowering expansion for that family

## Completed

- Closed the next Step 5 AArch64 synthetic switch seam behind the shared BIR
  boundary by widening the existing alloca-backed fence beyond the original
  two-case fixture:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving a
    single-case alloca-backed direct-LIR `switch` module still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of lowering
    through the generic AArch64 direct emitter
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject the broader
    synthetic alloca-backed switch-return family when every switch target is an
    empty return block, keeping that CFG ownership behind the shared BIR
    boundary without touching broader production switch handling
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Fenced the next narrow Step 5 AArch64 generic-emitter switch seam behind the
  shared BIR boundary:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving a
    constant-selector direct-LIR `switch` module still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of lowering
    through the generic AArch64 direct emitter
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject only that narrow
    synthetic constant-selector switch-return fallback before
    `try_emit_general_lir_asm(...)` so Step 5 closes another audited switch CFG
    seam without broadening the prune to unrelated direct-LIR coverage
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 multi-block switch seam behind the shared BIR
  boundary:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving an
    alloca-backed `LirSwitch` module still misses `try_lower_to_bir(...)` and
    must now fail explicitly instead of reviving target-local switch lowering
    through `try_emit_general_lir_asm(...)`
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject only that narrow
    synthetic alloca-backed switch-return fallback before
    `try_emit_general_lir_asm(...)` so Step 5 closes the bounded CFG seam
    without regressing the broader production switch surface that still lowers
    natively today
  - re-ran `backend_bir_tests`, rebuilt the tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Pruned the redundant AArch64 direct-LIR minimal conditional-return helper now
  that the shared BIR route already owns that bounded branch-return CFG shape:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the direct
    emitter's minimal conditional-return LIR entrypoint lowers through
    `try_lower_to_bir(...)`, collapses to a shared one-block `bir.select`, and
    emits the shared BIR-owned select labels instead of the original direct-LIR
    branch block labels
  - removed the now-redundant
    `parse_minimal_conditional_return_slice(...)` fallback from
    `src/backend/aarch64/codegen/emit.cpp`'s direct-LIR helper path so Step 5
    no longer keeps a duplicate backend-owned branch-return island after the
    outer BIR retry
  - rebuilt `backend_bir_tests`, re-ran it, refreshed `test_fail_after.log`,
    and passed the regression guard with no new failures and no pass-count
    drop (`2809 -> 2809`)
- Finished the follow-up AArch64 helper re-audit after that prune:
  - deleted the now-unused direct-LIR
    `parse_minimal_conditional_return_slice(const LirModule&)` helper from
    `src/backend/aarch64/codegen/emit.cpp`; the direct emitter no longer calls
    it after the shared BIR retry took ownership of that bounded branch-return
    shape
  - deleted the adjacent unused `MinimalConditionalPhiJoinSlice` scaffolding in
    the same file, which had become dead once the direct emitter started
    rejecting target-local phi revival after BIR misses
  - next audit target remains the generic AArch64 direct emitter's broader
    multi-block CFG surface rather than any remaining dedicated helper parser
  - rebuilt the tree, re-ran `backend_bir_tests`, refreshed
    `test_fail_after.log`, and kept the full-suite baseline flat at 32 failing
    tests out of 2841 with no new failures
- Closed the branch-only constant-return Step 5 CFG seam behind shared BIR:
  - taught `src/backend/lowering/lir_to_bir.cpp` to collapse the existing
    `goto`-only empty-block constant-return LIR shape into the canonical
    single-block immediate-return BIR form instead of leaving it stranded on
    target-local direct-LIR handling
  - added focused lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving
    `try_lower_to_bir(...)` now accepts that branch-only constant-return slice
    and normalizes it to one shared `bir.ret i32 9` block
  - updated
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` so the native
    emitters now must route the old `goto`-only constant-return fixture through
    the shared BIR-owned immediate-return path instead of target-local
    direct-LIR CFG behavior
  - re-ran `backend_bir_tests`, rebuilt the tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the matching AArch64 Step 5 direct-LIR parity gap for the remaining
  double-indirect local-pointer conditional-return CFG fallback:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the
    double-indirect local-pointer conditional-return fixture still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of reviving
    target-local multi-block CFG handling
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject that narrow
    fallback shape before `try_emit_general_lir_asm(...)` can emit it, keeping
    ownership behind the shared BIR boundary after the route misses lowering
  - rebuilt `backend_bir_tests`, re-ran it, refreshed `test_fail_after.log`,
    and passed the regression guard with no new failures and no pass-count
    drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 target-entrypoint CFG asymmetry without
  regressing the broader direct-LIR surface:
  - restricted `src/backend/aarch64/codegen/emit.cpp`'s
    `try_constant_fold_single_block(...)` helper to actual one-block modules so
    the old branch-following `goto`-only constant-return fallback no longer
    folds multi-block CFG chains after they miss shared BIR lowering
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` that constructs a
    `goto`-only constant-return LIR module and proves the AArch64 direct
    emitter now stays on the explicit branch-emission path instead of
    collapsing the CFG to an immediate-return constant-fold
  - re-ran `backend_bir_tests`, rebuilt the tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the x86 target-entrypoint preparation gap for Step 5 CFG ownership:
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` proving a minimal
    helper-call module with a dead entry alloca still misses raw
    `try_lower_to_bir(...)` but must route through the native x86 backend after
    backend-side pruning
  - taught `src/backend/x86/codegen/emit.cpp` to prune dead entry allocas
    before both the shared BIR retry and the direct-LIR fallback so x86 no
    longer diverges from the AArch64 entrypoint on this ownership seam
  - re-ran `backend_bir_tests` for the x86 entrypoint-pruning slice before the
    full-suite regression pass
  - regenerated the slice-local full-suite baseline and after-log on the
    rebuilt tree, then passed the regression guard with no new failures and no
    pass-count drop (`2809 -> 2809`); the standing 32 failures remain the same
    pre-existing AArch64 variadic set in both logs
- Audited the remaining Step 5 AArch64 target-local phi seam and chose the
  direct production prune:
  - rejected `LirPhiOp` in
    `src/backend/aarch64/codegen/emit.cpp`'s general direct-LIR emitter so
    modules that miss `try_lower_to_bir(...)` can no longer revive
    predecessor-copy phi lowering past the shared BIR boundary
  - deleted the now-dead AArch64 predecessor-copy collection/emission scaffold
    from the general direct-LIR emitter
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` that constructs an
    alloca-backed conditional-phi join which bypasses `try_lower_to_bir(...)`
    and now must fail the AArch64 direct emitter instead of silently lowering
    through the old target-local phi-copy path
  - re-ran `backend_bir_tests` for the AArch64 prune slice before the full-suite
    regression pass
- Audited the remaining Step 5 target-local phi seams and chose the smaller
  production prune for x86:
  - removed `LirPhiOp` interpretation from
    `src/backend/x86/codegen/emit.cpp`'s
    `parse_minimal_double_indirect_local_pointer_conditional_return_imm(...)`
    helper so the legacy direct-LIR constant-folder can no longer revive
    phi-aware CFG ownership after a module misses shared BIR lowering
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that constructs an
    alloca-backed conditional-phi join which bypasses `try_lower_to_bir(...)`
    and now must fail the x86 direct emitter instead of silently constant
    folding through the legacy phi path
  - re-ran `backend_bir_tests` for the x86 prune slice before the full-suite
    regression pass
- Replaced the mislabeled native Step 5 conditional-phi route coverage with a
  real lowerable LIR phi-join fixture in
  `tests/backend/backend_bir_test_support.cpp` and
  `tests/backend/backend_bir_test_support.hpp` so the pipeline tests no longer
  exercise only the already-lowered direct-BIR path
- Switched the x86 and AArch64 native conditional-phi pipeline tests in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
  `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` to use that LIR
  fixture and tightened the route assertions so they require the shared BIR
  select labels while rejecting the old target-local predecessor labels
- Re-ran `backend_bir_tests`, refreshed `test_fail_after.log`, and passed the
  regression guard with no new failures and no pass-count drop (`2841 ->
  2841`)
- Closed the remaining Step 5 x86 route-coverage gap in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` by proving the direct
  `c4c::backend::x86::emit_module(...)` LIR entrypoint itself routes the
  lowerable conditional-phi join and `u8` post-join-add slices through the
  shared BIR-owned select/join path instead of only covering the generic
  `c4c::backend::emit_module(...)` front door
- Re-ran `backend_bir_tests`, refreshed `test_fail_after.log`, and passed the
  regression guard with no new failures and no pass-count drop (`2841 ->
  2841`)
- Activated `plan.md` from
  `ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md`
- Reset active execution tracking for the new plan
- Audited the current typed-type boundary and backend ownership seams:
  - `src/codegen/lir/types.hpp` still stores only text plus coarse
    `LirTypeKind`
  - `src/codegen/lir/verify.cpp` only validates emptiness and `void` exclusion
    for `LirTypeRef`
  - `src/backend/lowering/lir_to_bir.cpp` still uses
    `lower_scalar_type_text(std::string_view)` at the main scalar cast/binop/cmp
    /select/return lowering sites
  - `src/backend/lowering/lir_to_bir.cpp` still parses function-signature
    parameter text in the fallback path
  - LIR-owned backend analysis remains live in `src/backend/liveness.cpp`,
    `src/backend/regalloc.cpp`, `src/backend/generation.cpp`, and
    `src/backend/stack_layout/*.cpp`
  - target-local phi cleanup is still present in
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp`
- Added exact integer-width payload support to `src/codegen/lir/types.hpp`
  through typed `LirTypeRef` integer metadata plus `LirTypeRef::integer(...)`
- Taught `src/codegen/lir/verify.cpp` to reject integer typed/text drift such
  as width/text mismatches
- Switched the first straight-line scalar lowering sites in
  `src/backend/lowering/lir_to_bir.cpp` from string decoding to typed
  `LirTypeRef` inspection for cast/binop/cmp/select/ret lowering
- Added focused backend tests that cover:
  - typed integer verifier mismatch rejection
  - typed `LirTypeRef::integer(8)` lowering through the existing i8 multiply
    BIR slice
- Converted the conditional select compare/cast lowering paths in
  `src/backend/lowering/lir_to_bir.cpp` from string-based integer decoding to
  typed `LirTypeRef` inspection
- Moved the two-parameter unsigned-char select-phi fixture in
  `tests/backend/backend_bir_test_support.cpp` onto explicit typed
  `LirTypeRef::integer(...)` constructors for its cast/cmp/phi/ret type refs
- Added a focused typed `i8` select regression in
  `tests/backend/backend_bir_lowering_tests.cpp` and kept it on the verified
  BIR path with `c4c::codegen::lir::verify_module(...)`
- Captured full-suite before/after logs in `test_fail_before.log` and
  `test_fail_after.log`
- Passed the regression guard check with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Replaced the remaining `lower_function_params(...)` signature-parameter
  scalar fallback from raw `i8`/`i32`/`i64` string matching to
  `LirTypeRef`-based typed lowering
- Moved the nearby one-parameter `widen_unsigned` `i8` zext BIR fixture onto
  explicit typed `LirTypeRef::integer(...)` cast construction
- Added focused typed zext lowering coverage for the one-parameter `i8`
  straight-line slice and kept it on the verified LIR path
- Re-ran `backend_bir_tests`, nearby backend route tests, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `try_lower_minimal_folded_two_arg_direct_call_module(...)` in
  `src/backend/lowering/lir_to_bir.cpp` to accept structured `LirFunction`
  helper return/param metadata when present instead of hard-requiring raw
  `"i32"` helper signature parameter text
- Replaced the folded-helper binop type check in
  `src/backend/lowering/lir_to_bir.cpp` from raw `"i32"` string comparison to
  typed `LirTypeRef` scalar inspection
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the folded
  two-argument direct-call path working when typed helper params disagree with
  stale signature param text
- Re-ran the focused direct-call backend BIR coverage plus the full suite and
  kept the regression guard passing (`2841 -> 2841`)
- Switched `parse_backend_single_identity_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the identity and
  dual-identity direct-call lowering paths working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `parse_backend_single_add_imm_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the add-immediate
  and call-crossing direct-call lowering paths working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests` plus the full suite and kept the regression guard
  passing (`2841 -> 2841`)
- Switched `parse_backend_two_param_add_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the minimal
  two-argument direct-call lowering path working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `parse_backend_single_param_slot_add_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the slot-based
  add-immediate direct-call lowering path working when the typed helper param
  disagrees with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched the remaining direct-call helper recognizer typed-operation checks
  in `src/backend/lowering/call_decode.hpp` from raw `"i32"` string matching to
  semantic `LirTypeRef` integer-width inspection for loads, stores, allocas,
  and helper add-rewrite operations
- Added focused shared-util regressions in
  `tests/backend/backend_shared_util_tests.cpp` that keep the single-add,
  slot-add, identity, and two-param helper recognizers working when helper
  operations are built from typed `LirTypeRef::integer(32)` metadata while the
  signature text stays stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched the remaining direct-call typed-operation checks in
  `src/backend/lowering/lir_to_bir.cpp` from raw `"i32"` string matching to
  semantic `LirTypeRef` integer-width inspection for direct-call local slots,
  caller stores, dual-identity subtraction, and call-crossing add chains
- Tightened the direct-call typed-operation regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_shared_util_tests.cpp` so the helper/caller
  `LirTypeRef` objects carry semantic `i32` metadata while their stored text is
  intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Completed the active helper-recognizer audit:
  - no remaining minimal direct-call helper recognizers still rely on
    signature-text parameter typing where structured helper metadata already
    exists
  - pointer payload support is not required for the next lowering slice
    because the remaining typed-lowering/direct-call seams in scope are still
    integer and signature-shape driven; pointer handling remains confined to
    declared-extern parsing and can stay deferred until the first
    pointer-backed BIR consumer is converted
- Switched the widened `i8` instruction-local cast/cmp/phi checks in
  `src/backend/lowering/lir_to_bir.cpp` from raw `i1`/`i8`/`i32` text matching
  to semantic integer-width inspection, including the nearby generic compare
  materialization and conditional-select zext guards
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the widened `i8`
  add, compare-return, and two-parameter select-phi BIR lowering paths working
  when instruction-local `LirTypeRef` objects carry semantic integer widths
  while their stored text is intentionally stale
- Re-ran `backend_bir_tests` plus the full suite and kept the regression guard
  passing (`2841 -> 2841`)
- Switched the widened `i8` add/compare/select helpers in
  `src/backend/lowering/lir_to_bir.cpp` off raw `LirRet.type_str == "i8"`
  gating and onto semantic `LirFunction.return_type` integer-width inspection
- Extended focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` so widened `i8`
  add-return, compare-return, branch-return select, and phi-select lowering
  still succeed when the typed instruction/function metadata is correct but the
  stored return text is intentionally stale
- Re-ran `backend_bir_tests` and `backend_shared_util_tests` for the widened
  `i8` stale-return-text slice before the full-suite check
- Re-ran the full suite into `test_fail_after.log` and passed the regression
  guard with no new failures and no pass-count drop (`2841 -> 2841`)
- Switched the generic single-block fallback in
  `src/backend/lowering/lir_to_bir.cpp` to derive the lowered function return
  type from structured `LirFunction.return_type` metadata first, then bounded
  signature text fallback, before trusting raw `LirRet.type_str`
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the generic
  straight-line add, compare-return, branch-return select, and phi-select BIR
  lowering paths working when instruction-local return/type text is stale but
  the enclosing function or typed `LirTypeRef` metadata still carries the
  correct `i32` semantics
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  and passed the regression guard with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Switched the zero-arg direct-call helper/caller recognizers in
  `src/backend/lowering/call_decode.hpp` and
  `src/backend/lowering/lir_to_bir.cpp` off hard `define i32` / `ret i32`
  gating and onto semantic `LirFunction.return_type` metadata first, with the
  existing signature/ret text fallback still available for untyped cases
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the zero-arg direct
  call lowering path working when either the helper or caller carries semantic
  `i32` return metadata while the stored signature and `ret` text are
  intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the remaining minimal direct-call caller-return gates in
  `src/backend/lowering/lir_to_bir.cpp` from raw `define i32` / `ret i32`
  text checks to `lir_function_matches_minimal_no_param_integer_return(...)`
  plus `lower_function_return_type(...)` for the two-argument, add-immediate,
  identity, dual-identity subtraction, and call-crossing slices
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep those direct-call
  slices lowering when the caller `LirFunction.return_type` still carries `i32`
  semantics but the stored caller signature and `ret` render text are
  intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the minimal extern global-array load and char/int pointer-difference
  recognizers in `src/backend/lowering/lir_to_bir.cpp` off raw
  cast/GEP/binop/cmp/load/ret integer text checks and onto semantic
  `LirTypeRef` width inspection plus structured function return metadata, while
  keeping bounded text fallback for untyped cases
- Completed the first Step 5 AArch64 ownership move:
  - audited `src/backend/backend.cpp`, `src/backend/aarch64/codegen/emit.cpp`,
    and `src/backend/x86/codegen/emit.cpp` for where lowerable phi/select CFG
    shapes still bypass the canonical BIR route
  - found that `src/backend/aarch64/codegen/emit.cpp` still attempted
    target-local direct LIR emission before `try_lower_to_bir(...)`, unlike the
    shared backend entrypoint and x86
  - reordered the AArch64 LIR entrypoint to prefer BIR emission when lowering
    and native BIR emission succeed, while preserving the existing direct-LIR
    fallback when the lowered BIR module is outside the native AArch64 BIR
    subset
  - added an AArch64 backend pipeline regression in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` that proves a
    lowerable conditional phi join now routes through the shared BIR-owned
    select path instead of staying on target-local phi predecessor-copy labels
- Re-ran `backend_bir_tests`, the four AArch64 route/c-testsuite regressions
  uncovered by the entrypoint reordering, and the full suite
- Refreshed `test_fail_after.log` and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the extern
  global-array load and both pointer-difference slices lowering when the
  instruction-local cast/GEP/binop/cmp text and `ret` text are intentionally
  stale but the typed metadata still carries the correct `i8`/`i32`/`i64`
  semantics
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the minimal scalar global-load, extern scalar global-load, and
  scalar global store-reload recognizers in
  `src/backend/lowering/lir_to_bir.cpp` off raw global/load/store/ret `"i32"`
  string checks and onto semantic global `TypeSpec`, instruction `LirTypeRef`,
  and function return metadata with the existing text fallback still available
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep those three scalar
  global slices lowering when the global text, load/store `LirTypeRef` render
  text, and `ret` render text are intentionally stale but the typed metadata
  still says `i32`
- Switched the declared direct-call fixed-param vararg comparison path in
  `src/backend/lowering/lir_to_bir.cpp` to recover fixed call-surface types
  from the backend direct-global typed-call parser when the generic LIR typed
  call parse rejects stale vararg suffix text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the declared
  direct-call vararg slice lowering when the stored fixed-param suffix text is
  stale but the actual typed call argument still carries the correct fixed
  `i32` surface
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Re-ran `backend_bir_tests` and the full suite, refreshed
  `test_fail_after.log`, and passed the regression guard with no new failures
  and no pass-count drop (`2841 -> 2841`)
- Switched the generic conditional branch-select and phi-select fast paths in
  `src/backend/lowering/lir_to_bir.cpp` off raw `ret.type_str` /
  `phi.type_str` text equality and onto typed scalar checks via
  `lower_function_return_type(...)` and semantic `LirTypeRef` inspection
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the generic
  one-parameter branch-select and phi-select lowering paths working when the
  branch/join return text or phi render text is stale but the typed semantics
  still match the enclosing function
- Re-ran `backend_bir_tests` and `backend_shared_util_tests` for the generic
  select stale-text slice before the full-suite check
- Switched the minimal countdown-loop recognizer in
  `src/backend/lowering/lir_to_bir.cpp` off raw instruction-local/load/store
  /ret `"i32"` text checks and onto semantic `LirTypeRef` integer-width
  inspection plus `lower_function_return_type(...)` for the loop exit return
- Switched the minimal declared direct-call lowering path in
  `src/backend/lowering/lir_to_bir.cpp` off raw caller `define i32`, `ret i32`,
  call `i32`, and declared-callee `declare i32` gates when structured
  `LirFunction` / `LirCallOp` metadata is already present
- Taught `src/codegen/lir/hir_to_lir.cpp` to preserve declaration
  `return_type` and `params` metadata on `LirFunction` declarations instead of
  leaving declaration functions text-only
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the zero-arg
  declared direct-call lowering path working when the caller, declared callee,
  and call instruction carry semantic `i32` metadata while the stored
  signature and return text are intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the bounded
  countdown do-while lowering path working when the loop-local alloca,
  load/store/binop/cmp type refs carry semantic `i32` metadata while their
  stored text, including the exit `ret`, is intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  and passed the regression guard with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Added typed `LirExternDecl.return_type` metadata in
  `src/codegen/lir/ir.hpp`, taught `src/codegen/lir/hir_to_lir.cpp` to
  populate it from the existing extern declaration map, and switched the
  minimal declared direct-call extern gate in
  `src/backend/lowering/lir_to_bir.cpp` to prefer that structured return
  metadata before falling back to raw `return_type_str`
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the inferred-param
  extern declared direct-call lowering path working when the extern decl text
  and call/return render text are intentionally stale but the typed extern and
  call metadata still carry `i32` semantics
- Re-ran `backend_bir_tests` and `backend_shared_util_tests` for the typed
  extern declared-direct-call slice before the full-suite check
- Re-ran the full suite into `test_fail_after.log` and passed the regression
  guard with no new failures and no pass-count drop (`2841 -> 2841`)
- Switched the remaining minimal-helper return gates in
  `src/backend/lowering/call_decode.hpp` from raw `ret.type_str == "i32"`
  checks to `backend_lir_lower_function_return_type(...)` for the
  add-immediate, slot-add, identity, and two-parameter helper recognizers
- Added focused shared-util regressions in
  `tests/backend/backend_shared_util_tests.cpp` that keep those helper
  recognizers accepting semantic `i32` helpers when the stored helper return
  text is intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the backend direct-global typed-call fallback in
  `src/backend/lowering/call_decode.cpp` to return the backend-owned parsed
  view and preserve fixed vararg parameter types from the actual typed call
  args instead of rejecting the parse on stale fixed-param suffix text
- Added a focused shared-util regression in
  `tests/backend/backend_shared_util_tests.cpp` that keeps the direct-global
  typed-call operand helper recovering the fixed typed vararg operand when the
  stored fixed-param suffix text is intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Removed the remaining zero-fixed-param declared direct-call fallback gate in
  `src/backend/lowering/lir_to_bir.cpp` that still required raw
  `declare i32 @name()` signature text in the vararg/no-typed-param branch
  even when typed callee return metadata and parsed fixed params already proved
  the declaration surface
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the zero-arg
  declared direct-call vararg slice lowering when the callee carries semantic
  `i32` return metadata while the stored declaration text is intentionally
  stale (`declare i64 @helper_decl(...)`)
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the remaining caller-local-slot `i32` recognizer guards in
  `src/backend/lowering/call_decode.hpp` and
  `src/backend/lowering/lir_to_bir.cpp` from typed-only checks to semantic
  integer-width matching with bounded `i32` text fallback for stale caller
  alloca/store/load render text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the two-argument
  local-slot direct-call slice and the add-immediate local-slot direct-call
  slice lowering when the caller alloca/store/load text is stale but typed
  metadata still carries `i32`
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Completed the remaining Step 1 helper re-audit in
  `src/backend/lowering/lir_to_bir.cpp`:
  - `lower_binary(...)`, `lower_compare_materialization(...)`, and
    `lower_select_materialization(...)` already lower scalar kinds from
    semantic `LirTypeRef` width metadata through
    `lower_scalar_type(const LirTypeRef&)`
  - the remaining raw-text sites in `lir_to_bir.cpp` are no longer generic
    scalar/materialization helpers; they are specific fallback recognizers or
    backend-ownership seams that belong to the next migration stage
  - no additional generic typed-type conversion remains for Step 1, so the plan
    now advances to BIR-owned phi/CFG normalization instead of stretching the
    typed-type audit further
- Added x86 and AArch64 end-to-end LIR route coverage in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
  `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` for the bounded `u8`
  predecessor-add phi-with-post-join-add slice by retargeting the existing LIR
  fixture onto each native backend triple and asserting native emission stays
  on the shared BIR-owned `.Lselect_*` / `.Lselect_join` path instead of
  target-local predecessor labels
- Rebuilt `backend_bir_tests`, re-ran it, re-ran the full suite into
  `test_fail_after.log`, and passed the regression guard in
  `--allow-non-decreasing-passed` mode with no new failures and no pass-count
  drop (`2841 -> 2841`)
- Pruned the remaining x86 CFG-sensitive direct-LIR helper stubs in
  `src/backend/x86/codegen/emit.cpp` by removing the `goto-only` branch walker
  and double-indirect local-pointer conditional-return constant-fold fallback
  from the production direct-emitter entrypoint once those modules miss shared
  BIR lowering
- Added focused regressions in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that prove both shapes
  now fail with the normal direct-LIR unsupported error instead of reviving
  target-local CFG ownership past the BIR boundary
- Rebuilt `backend_bir_tests`, re-ran it, refreshed `test_fail_after.log`, and
  passed the regression guard with no new failures and no pass-count drop
  (`2809 -> 2809` passes, same 32 pre-existing AArch64 variadic/backend
  failures)

## Next

- audit `src/backend/backend.cpp`, `src/backend/aarch64/codegen/emit.cpp`, and
  `src/backend/x86/codegen/emit.cpp` to classify which phi/CFG-sensitive cases
  still bypass the canonical BIR route or still require target-local cleanup
- choose the narrowest production slice that can move one of those remaining
  phi/CFG responsibilities behind `try_lower_to_bir(...)` without expanding
  scope into MIR-owned liveness/regalloc work
- keep pointer payload support deferred until a concrete pointer-backed BIR
  lowering consumer appears, then add only the narrow typed payload that
  consumer needs

## Blockers

- none recorded

## Resume Notes

- Repo lifecycle state was `no active plan`; this todo starts from activation.
- The source idea fixes migration order as:
  1. typed LIR type ownership
  2. canonical BIR phi/control-flow surface
  3. MIR-owned liveness
  4. MIR-owned regalloc and stack-layout integration
  5. delete temporary LIR-side backend shims
- Use `todo.md` as the canonical execution state even though the reusable skill
  text mentions `plan_todo.md`.
- Immediate validating targets:
  `test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper_with_stale_return_text`
  `test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper_with_stale_return_text`
  `test_backend_shared_call_decode_accepts_typed_i32_identity_helper_with_stale_return_text`
  `test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper_with_stale_return_text`
  and
  `test_lir_verify_rejects_typed_integer_text_mismatch` and
  `test_bir_lowering_accepts_typed_tiny_return_add_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_tiny_return_ne_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_i8_return_add_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_i8_return_ne_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_return_text`
  `test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_phi_text`
  `test_bir_lowering_accepts_typed_two_param_u8_select_ne_branch_slice_with_stale_text`
  and
  `test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text`
  and
  `test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text`
  in
  `tests/backend/backend_bir_lowering_tests.cpp`.
- Latest validating targets for the direct-call helper seam:
  `test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_helper_param`
  `test_bir_lowering_accepts_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param`
  and
  `test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param`
  plus
  `test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_helper_params`
  in `tests/backend/backend_bir_lowering_tests.cpp`.
- Latest validating targets for the typed helper-operation seam:
  `test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper`
  `test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper`
  `test_backend_shared_call_decode_accepts_typed_i32_identity_helper`
  and
  `test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper`
  in `tests/backend/backend_shared_util_tests.cpp`.
- Latest validating target for the direct-global vararg typed-call seam:
  `test_backend_shared_call_decode_prefers_typed_vararg_call_args_over_stale_fixed_suffix_text`
  in `tests/backend/backend_shared_util_tests.cpp`.
- Latest validating target for the declared direct-call vararg lowering seam:
  `test_bir_lowering_uses_actual_fixed_vararg_call_types_for_declared_direct_calls`
  in `tests/backend/backend_bir_lowering_tests.cpp`.
- Full-suite before/after comparison for this slice should use
  `test_fail_before.log` and `test_fail_after.log` with the regression-guard
  checker script.
