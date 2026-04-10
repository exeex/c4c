# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 4 continue transferring one bounded x86 top-level ownership seam out of
  `emit.cpp` after the repeated `printf` immediate and local-buffer
  `strcpy`/`printf` direct-LIR routes moved into
  `src/backend/x86/codegen/direct_printf.cpp`
- immediate target:
  continue Step 4 in `src/backend/x86/codegen/direct_calls.cpp` after the
  bounded second-local two-argument helper-call and second-local rewrite
  routes by moving the remaining local-slot-fed rewrite variants in bounded
  sub-slices, starting with the first-local rewrite route
  - keep translated `variadic.cpp` itself parked until `X86Codegen` grows the
    state/method surface it still references
  - keep `frame_compact.cpp` parked until a future iteration can prove a real
    shrink/rewrite shape instead of enabling the current placeholder pass

## Next Slice

- continue Step 4 by identifying the next bounded top-level x86 ownership
  transfer that can compile cleanly against the current transitional headers
  after `direct_printf.cpp` joins `direct_variadic.cpp` and
  `direct_globals.cpp` as another reachable Step 4 sibling seam
- with the repeated `printf`, local-buffer `strcpy`/`printf`, counted ternary
  loop, and `string_literal_char` routes now behind `direct_printf.cpp`,
  continue with the prepared-LIR call/helper routes in `emit.cpp`, starting
  with the bounded `void` helper/call ownership seam before considering the
  larger integer helper-call cluster
- after the bounded `void` seam now lives in `src/backend/x86/codegen/direct_void.cpp`,
  continue Step 4 with the neighboring prepared-LIR integer helper-call routes
  in bounded sub-slices instead of widening unrelated x86 emitter support
- after the bounded `param_slot_add` plus zero-arg extern/declared-call routes
  now live in `src/backend/x86/codegen/direct_calls.cpp`, continue Step 4 with
  the remaining prepared-LIR integer helper-call routes there: the single-local
  helper call first, then the two-arg helper variants
- after the bounded single-local helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp`, continue Step 4 there with the
  neighboring immediate/immediate two-argument helper-call route before moving
  to the local-slot-fed two-argument variants
- after the bounded immediate/immediate two-argument helper-call route now
  lives in `src/backend/x86/codegen/direct_calls.cpp`, continue Step 4 there
  with the first-local two-argument helper-call route before the remaining
  second-local and rewrite variants
- after the bounded first-local two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp`, continue Step 4 there with the
  second-local two-argument helper-call route before the rewrite variants
- after the bounded second-local two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp`, continue Step 4 there with the
  second-local rewrite helper-call route before the remaining rewrite variants
- after the bounded second-local rewrite helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp`, continue Step 4 there with the
  first-local rewrite helper-call route before the both-local rewrite variants
- keep translated `variadic.cpp` parked until a future iteration can expose
  the missing `X86Codegen` state/method surface intentionally instead of
  compiling placeholder member bodies by accident
- keep `frame_compact.cpp` parked until dead-store and callee-save cleanup
  expose a concrete safe shrink shape instead of enabling the placeholder pass
  by default
- keep the full translated top-level units parked when they still depend on
  missing `X86Codegen` state instead of forcing broad header/class expansion
- prefer small sibling-file ownership moves like the direct global seam when a
  full translated unit is not yet compile-ready

## Current Iteration Notes

- this plan was activated by explicit user priority override
- idea 44 remains open as the parked shared-BIR cleanup and legacy-matcher
  consolidation lane
- this iteration extends Step 4 with another bounded prepared-LIR sibling seam:
  the x86 `void` helper/call direct-LIR routes now live in
  `src/backend/x86/codegen/direct_void.cpp` instead of `emit.cpp`
- this iteration starts the neighboring integer helper-call cluster with a new
  bounded sibling seam: the x86 prepared-LIR `param_slot_add` route plus the
  zero-arg extern/declared-call route now live in
  `src/backend/x86/codegen/direct_calls.cpp` instead of `emit.cpp`
- this iteration extends that direct-calls sibling seam one bounded step
  further: the x86 prepared-LIR single-local helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR immediate/immediate
  two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR first-local two-argument
  helper-call route now lives in `src/backend/x86/codegen/direct_calls.cpp`
  instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR second-local two-argument
  helper-call route now lives in `src/backend/x86/codegen/direct_calls.cpp`
  instead of staying in `emit.cpp`
- this iteration continues that direct-calls sibling seam with the next
  bounded ownership move: the x86 prepared-LIR second-local rewrite
  two-argument helper-call route now lives in
  `src/backend/x86/codegen/direct_calls.cpp` instead of staying in `emit.cpp`
- added a focused backend regression that calls the moved second-local rewrite
  two-arg helper seam explicitly so the Step 4 ownership move stays observable
  apart from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_rewrite_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice`
  and `test_x86_direct_call_helper_accepts_two_arg_second_local_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and no new `>30s` cases
- added a focused backend regression that calls the moved second-local two-arg
  helper seam explicitly so the Step 4 ownership move stays observable apart
  from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_second_local_arg_call_slice`
  plus `test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice`,
  `test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice`, and
  `test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and no new `>30s` cases
- added a focused backend regression that calls the moved first-local two-arg
  helper seam explicitly so the Step 4 ownership move stays observable apart
  from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_local_arg_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice`
  and `test_x86_direct_call_helper_accepts_two_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and the existing `backend_bir_tests`
  `>30s` warning unchanged
- added a focused backend regression that calls the moved two-argument helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_two_arg_call_slice`
  plus `test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice` and
  `test_x86_direct_call_helper_accepts_local_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- added a focused backend regression that calls the moved single-local helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_local_arg_call_slice`
  plus `test_x86_direct_call_helper_accepts_param_slot_add_slice` and
  `test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- added a focused backend regression that calls the new direct call-helper seam
  explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_call_helper_accepts_param_slot_add_slice`
  plus `test_x86_direct_emitter_lowers_minimal_param_slot_add_slice` and
  `test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice`
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests and the existing `backend_bir_tests`
  `>30s` warning unchanged
- the moved seam covers the helper-only `voidfn` body, the two-function
  `voidfn(); return imm;` slice, and the main-only extern-void call plus
  immediate return shape without widening the surrounding integer helper-call
  ownership surface yet
- added a focused backend regression that calls the new direct `void` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_void_helper_accepts_void_direct_call_return_slice`
  plus the existing `00080` helper-only, helper-call, and main-only direct
  x86 emitter regressions
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- this iteration extends the direct-printf sibling seam one bounded step
  further: the `string_literal_char` direct-LIR route now lives in
  `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that calls the moved direct
  `string_literal_char` helper seam explicitly so the Step 4 ownership move
  stays observable apart from the broader prepared-LIR dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_string_literal_char_slice`
  plus `test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly`
  and the existing repeated/local-buffer/counting direct-printf helper guards
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `1217` passed / `181` failed before versus `2723` passed / `181` failed
  after, with no newly failing tests
- this iteration extends the direct-printf sibling seam one bounded step
  further: the counted ternary-loop `printf` direct-LIR route now lives in
  `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that calls the new direct `printf` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader native-path dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_counted_printf_ternary_loop_slice`
  plus the existing local-buffer direct-printf helper guard and the native-path
  counted-ternary backend filter
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against a matched `HEAD` baseline after restoring the same external
  `c-testsuite` inventory in the clean worktree; the regression guard reported
  `181` failed before and after with no newly failing tests
- this iteration extends the direct-printf sibling seam one bounded step
  further: the local-buffer `strcpy` plus one-byte-offset `printf` direct-LIR
  route now lives in `src/backend/x86/codegen/direct_printf.cpp` instead of
  `emit.cpp`
- added a focused backend regression that calls the new direct `printf` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader native-path dispatcher coverage
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_local_buffer_string_copy_printf_slice`
  plus the existing minimal/source-like local-buffer native-path regressions
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
  and the existing `backend_bir_tests` `>30s` timeout warning unchanged
- this iteration extends Step 4 with the first dedicated direct-printf sibling
  seam: the bounded repeated `printf` immediate direct-LIR route now lives in
  `src/backend/x86/codegen/direct_printf.cpp` instead of `emit.cpp`
- added a focused backend regression that calls the new direct `printf` helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader x86 pipeline path
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_printf_helper_accepts_repeated_printf_immediates_slice`
  plus the existing source-like repeated-`printf` native-path regression and
  the direct variadic helper guard
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- this iteration extends Step 4 with another bounded sibling seam: the
  prepared direct-LIR variadic sum2 and variadic double-byte runtime slices now
  live in `src/backend/x86/codegen/direct_variadic.cpp` instead of `emit.cpp`
- the translated `src/backend/x86/codegen/variadic.cpp` unit stays parked:
  once it entered the build it immediately proved it still depends on
  non-exported `X86Codegen` state/method members, so the new direct sibling
  file narrows ownership transfer without widening that class surface
- added a focused backend regression that calls the new direct variadic helper
  seam explicitly so the Step 4 ownership move stays observable apart from the
  broader dispatcher path
- focused checks passed:
  `./build/backend_bir_tests test_x86_direct_variadic_helper_accepts_variadic_sum2_runtime_slice`
  plus the existing prepared variadic sum2/double-byte filters
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- the current question is not "what more should be translated"
- the current question is "which already-translated x86 codegen pieces can be
  made real and reachable first"
- Step 3 is now effectively complete apart from the intentionally parked
  `frame_compact.cpp` placeholder, so this iteration moves back to Step 4
  ownership transfer instead of widening the peephole matcher surface again
- this iteration extends the direct-global sibling seam one bounded step
  further: the direct-BIR global two-field struct store/sub/sub route now
  lives in `src/backend/x86/codegen/direct_globals.cpp` instead of `emit.cpp`
- added a new direct x86 BIR regression that pins the two-field global store,
  byte-offset reload, and final subtract chain so this Step 4 ownership move
  stays observable on the native path
- the focused `./build/backend_bir_tests global_two_field_struct_store_sub_sub`
  regression passed after the ownership move
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- this iteration extends the direct-global sibling seam one bounded step
  further: the two-function `store global in helper; return immediate in both
  helper and entry` route now lives in `src/backend/x86/codegen/direct_globals.cpp`
  instead of `emit.cpp`
- added a new direct x86 BIR regression that pins the helper store plus the
  independent entry return so this Step 4 ownership move stays observable on
  the native path
- the focused `./build/backend_bir_tests global_store_return_and_entry_return`
  regression passed after the ownership move
- the broad `ctest --test-dir build -j8 --output-on-failure` rerun remained
  monotonic against `test_fail_before.log`; the regression guard reported
  `2723` passed / `181` failed before and after with no newly failing tests
- this iteration moves the bounded scalar-global direct-emission seam out of
  `emit.cpp` into `src/backend/x86/codegen/direct_globals.cpp`, covering the
  native scalar global load, extern scalar global load, and scalar global
  store-reload paths
- the original translated `globals.cpp` unit still does not compile against the
  current transitional `X86Codegen` header because it references state/method
  members that are not live yet, so the new sibling file intentionally narrows
  ownership transfer without widening class-surface work
- added a new direct regression for the zero-initialized scalar global-load
  path so the new sibling seam is pinned for both `.data` and `.bss` routes
- targeted backend filters for the direct BIR scalar-global load/store paths
  and the shared-BIR scalar-global store-reload routes passed on the new seam
- the broad `ctest --test-dir build -j8 --output-on-failure` sweep still fails
  in the same pre-existing aggregate areas, starting with the existing
  `backend_bir_tests` segfault and the wider known x86/backend failure set
- this iteration activates the parked translated callee-save cleanup slice in
  the real x86 peephole build and post-pass orchestration without enabling the
  still-placeholder frame-compaction pass
- `peephole/passes/callee_saves.cpp` now compiles as part of the real build,
  runs after the existing optimization rounds, and removes an unused callee-
  saved register save even when an earlier live pass already deleted the
  matching restore
- the new direct regression keeps the current frame-size boundary explicit: the
  callee-save save/restore pair is removed, but `subq $N, %rsp` remains parked
  until a future frame-compaction slice owns stack-slot rewriting
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests;
  the guard again reported one `>30s` test (`backend_bir_tests`), matching the
  existing harness instability rather than a new regression
- current evidence shows most of `src/backend/x86/codegen/*.cpp` is still not
  in the build, while `emit.cpp` remains the practical owner of x86 emission
- the active inventory for this plan is now explicit inside idea 43:
  16 translated top-level codegen units plus the translated peephole subtree
- the translated peephole subtree is the first bounded reachable slice because
  it has a self-contained string-in/string-out seam even though the current
  translated parser helpers still need a shared header and the emitted x86 asm
  uses Intel syntax rather than the ref tree's AT&T-shaped patterns
- the active x86 route now passes through the translated peephole entrypoint in
  both the direct x86 wrapper and the shared backend assembly handoff path
- the first live optimization is syntax-agnostic redundant jump-to-immediately-
  following-label elimination; the rest of the translated local-pass inventory
  remains parked until its assumptions are reconciled with the current emitter
- the translated compare-and-branch pass needed one shared classifier seam
  before it became reachable: `types.cpp` now classifies `cmp*` lines as
  `LineKind::Cmp`, which lets the pass fuse cmp/setcc/test/jcc sequences on the
  real x86 peephole path
- the loop-trampoline coalescing slice exposed another operand-classification
  seam: many comma-separated AT&T instructions still leave `LineInfo.dest_reg`
  unset because the trailing operand is not trimmed before family lookup, so
  the live trampoline pass currently uses a local trimmed-destination fallback
  instead of broadening that shared parser mid-slice
- this iteration promotes that trailing-operand trim into the shared
  classifier/helper path because the next translated trampoline case
  (`movslq` copy-back) would otherwise duplicate the same local fallback again
- the aggregate `backend_bir_tests` runner still crashes in pre-existing typed
  LIR validation coverage before it can serve as a narrow peephole harness, so
  this iteration verified the new trampoline form with a focused standalone
  peephole harness while the full-suite regression guard remained monotonic
- the next bounded gap against the translated loop-trampoline behavior is
  mixed safe/unsafe shuffle handling: the current C++ pass still bails out of
  the whole trampoline when one sibling move cannot be coalesced, while the
  reference keeps the unsafe sibling parked and still applies the safe rewrite
- this iteration intentionally pivots away from that matcher growth lane:
  `dead_code.cpp` is already translated but still disconnected from the real
  orchestration/build, so Step 3 can make progress by activating another
  existing pass file with focused peephole-only coverage
- the translated dead-code slice landed as a bounded orchestration/build step:
  `peephole/passes/dead_code.cpp` now compiles into the real x86 backend and
  runs inside the live peephole round without widening unrelated matcher logic
- the aggregate `backend_bir_tests` runner still times out on pre-existing
  typed-LIR validation failures, so this iteration validated the new pass with
  direct filtered backend test invocations plus the full-suite regression guard
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests;
  the guard reported one `>30s` test (`backend_bir_tests`), matching the
  existing harness-timeout situation rather than a new regression
- this iteration closes the simplest parked stack-load gap: the live pass now
  recognizes a single-slot `%rax` spill in the predecessor plus an immediate
  trampoline reload/copy-back pair and rewrites that shape onto the
  loop-carried register without widening into generic spill analysis
- the next bounded gap after this iteration is the remaining stack-load family:
  wider spill forms should stay parked until their predecessor-store matching
  and fallthrough safety rules are explicit instead of inferred ad hoc
- this iteration closes one more bounded stack-load shape without widening the
  shared parser or the predecessor scan into generic spill analysis: the live
  pass now accepts a single-slot `%eax` spill plus trampoline reload and
  `movslq` copy-back onto the loop-carried register
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new regression, so this slice again used a focused
  standalone peephole harness while the full-suite regression guard stayed
  monotonic
- this iteration closes the next bounded stack-load family member without
  widening into generic spill analysis: the live pass now accepts a single-slot
  `%rax` spill whose trampoline reload writes the loop-carried register
  directly via `movq -N(%rbp), %loop_reg`
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new direct-reload regression, so this slice again used the
  standalone backend test filter plus the full-suite regression guard
- this iteration closes the direct sign-extending stack-reload variant without
  widening predecessor analysis: the live pass now accepts a single-slot
  `%eax` spill whose trampoline reload writes the loop-carried register
  directly via `movslq -N(%rbp), %loop_reg`
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new direct-`movslq` regression, so this slice again uses the
  standalone backend test filter/subset plus the full-suite regression guard
- this iteration deliberately adds a negative regression instead of widening
  stack-load matching again: the live x86 peephole must keep the spill,
  reload, and trampoline branch parked when the branch fallthrough still
  consumes `%rax` after the bounded predecessor spill
- the new stack-load safety regression passed on the existing implementation,
  confirming the current fallthrough guard already enforces that `%rax`/`%eax`
  boundary
- the first full-suite rerun picked up one extra unrelated failure in
  `cpp_typedef_owned_functional_cast_perf`; rerunning that test in isolation
  passed, and a second full-suite rerun returned to the monotonic baseline
- this iteration promotes copy propagation because the translated pass already
  exists, is more substantive than the parked stub passes, and composes
  directly with the live dead-move cleanup without crossing into epilogue or
  frame-layout ownership
- the translated copy-propagation slice landed as another bounded
  orchestration/build step: `peephole/passes/copy_propagation.cpp` now
  compiles into the real x86 backend and runs inside the live peephole round
  without widening into ABI-sensitive cleanup passes
- the new direct regression deliberately gives the existing dead-move cleanup
  explicit overwrite proof after the propagated use so this slice stays about
  activating translated copy propagation rather than broadening dead-code
  semantics
- the first full-suite rerun picked up one extra unrelated failure in
  `cpp_qualified_template_call_template_arg_perf`; rerunning that test in
  isolation passed, and a second full-suite rerun returned to the same
  `2723` passed / `181` failed count as the baseline
- the aggregate `backend_bir_tests` entry still does not serve as a narrow
  peephole harness: on the rerun it reached the same pre-existing
  `test_bir_lowering_accepts_local_two_field_struct_sub_sub_two_return_module`
  crash path quickly enough to surface as `SEGFAULT` instead of the earlier
  `Timeout`, so the unstable aggregate runner remains a known blocker rather
  than a new peephole regression
- this iteration activates the translated store-forwarding slice in the real
  x86 peephole build and optimization round with a focused regression proving a
  same-register `(%rbp)` reload is removed after a matching bounded stack
  store
- the store-forwarding regression initially exposed a shared classifier seam:
  `types.cpp` left stack-store offsets contaminated by the source operand and
  left stack-load destination registers with leading whitespace, so matching
  `StoreRbp`/`LoadRbp` pairs never surfaced to the live pass until that parse
  cleanup landed
- this slice intentionally keeps dead-store semantics parked: the new
  regression only claims bounded reload elimination, while the surviving stack
  store remains acceptable because the current dead-store pass still treats
  function exit as a barrier
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests
  and no new `>30s` tests
- this iteration activates the parked translated tail-call slice in the real
  x86 peephole build and optimization round without widening into callee-save
  or frame-compaction work
- `peephole/passes/tail_call.cpp` now compiles as part of the real build,
  runs inside the live optimization loop, and rewrites a bounded
  `call; pure epilogue; ret` sequence into the same epilogue followed by
  `jmp target`
- the first full-suite rerun picked up the same intermittent
  `cpp_qualified_template_call_template_arg_perf` outlier already noted in
  earlier slices; rerunning that test in isolation passed, and a second
  full-suite rerun returned to the baseline `2723` passed / `181` failed
  counts with no newly failing tests
- this slice intentionally keeps the shared classifier boundary parked:
  focused coverage uses the currently classified `call ...` spelling rather
  than widening `types.cpp` to classify `callq ...` in the same iteration
- this iteration activates the translated memory-fold slice in the real x86
  peephole build and optimization round with focused regressions around a
  bounded stack-load plus ALU-use pattern
- `peephole/passes/memory_fold.cpp` now compiles as part of the real build,
  runs inside the live optimization loop, and folds a safe `movq`/`movl`
  reload from `(%rbp)` into the following ALU source operand when the loaded
  register is a scratch register and is not also the ALU destination
- the direct regression keeps one safety boundary explicit: the load remains
  parked when folding would turn the loaded register into the ALU destination,
  which would change the instruction into an invalid memory-destination shape
- the aggregate `backend_bir_tests` entry remains an unstable broad harness:
  this iteration's focused filters passed, the full-suite fail set stayed at
  `2723` passed / `181` failed, and the monotonic guard passed only in
  non-decreasing mode because the existing aggregate backend runner surfaced as
  `Timeout` instead of the baseline run's `SEGFAULT`

## Recently Completed

- moved the bounded direct-BIR global two-field struct store/sub/sub route out
  of `emit.cpp` into `src/backend/x86/codegen/direct_globals.cpp`
- added a direct x86 BIR regression that pins the native two-field global
  store/sub/sub route without relying on the LIR-to-BIR lowering hop
- extended `src/backend/x86/codegen/direct_globals.cpp` to own the bounded
  two-function global store plus independent entry-return direct-BIR route
- moved `MinimalGlobalStoreReturnAndEntryReturnSlice` ownership out of
  `emit.cpp` into `x86_codegen.hpp` so the sibling direct-global unit can own
  that emitter seam cleanly
- added a direct x86 BIR regression for the helper-global-store plus entry-
  return route and wired it into `backend_bir_tests`
- started Step 4 ownership transfer after the translated peephole subtree
  reached the real x86 path
- moved the bounded scalar-global direct-emission cluster out of `emit.cpp`
  into `src/backend/x86/codegen/direct_globals.cpp`
- wired `direct_globals.cpp` into both the main x86 build and the backend test
  build
- added a direct regression that pins the zero-initialized scalar global-load
  path on the native x86 route
- created idea 43 to own x86 peephole pipeline completion
- parked idea 44 as a separate shared-BIR cleanup lane
- switched the active runbook and execution state to idea 43
- reprioritized idea 43 so integrating already-translated x86 codegen units is
  now ahead of peephole-only work
- added the first bounded translated x86 peephole compile-in cluster to the
  build: `peephole/mod.cpp`, `peephole/types.cpp`,
  `peephole/passes/helpers.cpp`, `peephole/passes/local_patterns.cpp`, and
  `peephole/passes/mod.cpp`
- added a shared peephole header so that cluster now compiles as a real unit
  instead of parked source inventory
- routed x86 emitted asm through `peephole_optimize(...)` on both the direct
  x86 wrapper path and the shared backend assembly handoff path
- added targeted backend tests that pin the live redundant-jump cleanup and
  confirm emitted countdown-loop asm reaches the peephole stage
- compiled the translated `peephole/passes/push_pop.cpp` unit into the real
  x86 backend and test builds
- wired the safe redundant `pushq`/`popq` elimination pass into the live x86
  peephole optimization loop
- added a direct regression test that proves the translated push/pop pass now
  removes a redundant pair while preserving the surrounding label and return
- compiled the translated `peephole/passes/compare_branch.cpp` unit into the
  real x86 backend and test builds
- wired the translated compare-and-branch fusion pass into the live x86
  peephole optimization loop
- added a direct regression test that proves the live x86 peephole now fuses a
  cmp/setcc/test/jne boolean-materialization sequence into a direct conditional
  jump
- compiled the translated `peephole/passes/loop_trampoline.cpp` unit into the
  real x86 backend and test builds
- wired a conservative single-entry trampoline rewrite into the live x86
  peephole optimization loop before the generic adjacent-jump cleanup
- added a direct regression test that proves the live x86 peephole now
  rewrites a sole incoming branch away from a label-only trampoline block onto
  the real loop header while preserving the current emitted label syntax
- widened the live translated loop-trampoline pass to coalesce a bounded
  register-register `movq` trampoline copy back into the predecessor update
  chain before redirecting the branch onto the real loop header
- added a direct regression test that proves the live x86 peephole now rewrites
  `movq %loop_reg, %tmp` / mutate `%tmp` / trampoline `movq %tmp, %loop_reg`
  into a direct mutation of the loop-carried register
- promoted trailing-operand trimming into the shared x86 peephole
  classifier/helper path so comma-delimited destinations no longer leave
  `LineInfo.dest_reg` unset just because the trailing operand still carries
  whitespace
- widened the live translated loop-trampoline pass to recognize and coalesce a
  bounded `movslq %tmpd, %loop_reg` trampoline copy-back onto the real
  loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  a 32-bit update feeding a `movslq` trampoline copy-back into a direct update
  of the loop-carried register
- widened the live translated loop-trampoline pass to coalesce a proven-safe
  register shuffle inside a mixed trampoline block even when a sibling move
  still has to stay parked
- taught the loop-trampoline predecessor scan to reject rewrites that would
  read the loop-carried destination while also writing the temporary, avoiding
  bogus self-referential rewrites such as `addq %r10, %r10`
- added a direct regression test that proves the live x86 peephole now removes
  only the safe `%r9/%r14` shuffle from a mixed trampoline while leaving the
  unsafe `%r10/%r15` pair and trampoline branch in place
- widened the live translated loop-trampoline pass to coalesce a conservative
  single-slot stack-spill trampoline when the predecessor computes into `%rax`,
  spills to one `(%rbp)` slot, and the trampoline immediately reloads `%rax`
  before copying back into the loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded stack-spill trampoline onto the loop-carried register and drops
  the now-dead spill, reload, and trampoline copy-back sequence
- widened the live translated loop-trampoline pass to coalesce a bounded
  32-bit single-slot stack spill when the predecessor computes through `%eax`,
  stores to one `(%rbp)` slot, and the trampoline reloads `%eax` before
  `movslq` copy-back into the loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded `movl`/`movslq` stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill, reload, and trampoline copy-back
  sequence
- widened the live translated loop-trampoline pass to coalesce a bounded
  64-bit single-slot stack spill when the trampoline reloads the loop-carried
  register directly from `(%rbp)` instead of bouncing through `%rax`
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded direct-reload stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill and direct trampoline reload
- widened the live translated loop-trampoline pass to coalesce a bounded
  32-bit single-slot stack spill when the trampoline sign-extends directly from
  `(%rbp)` into the loop-carried register instead of reloading `%eax` first
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded direct `movslq` stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill and direct trampoline reload
- added a direct regression test that proves the live x86 peephole keeps a
  bounded stack-spill trampoline parked when the branch fallthrough still
  consumes `%rax`, locking in the current fallthrough-safety boundary instead
  of widening the matcher
- rebuilt and reran the full ctest suite with monotonic results:
  `183` failures before, `181` failures after, no newly failing tests
- rebuilt and reran the full ctest suite for this test-only safety slice with
  monotonic results: `181` failures before, `181` failures after, no newly
  failing tests
- compiled the translated `peephole/passes/dead_code.cpp` unit into the real
  x86 backend and test builds
- wired the translated dead register-move and overwritten stack-store cleanup
  passes into the live x86 peephole optimization loop
- added direct regression tests that prove the live x86 peephole now drops a
  dead `movq` whose destination is overwritten before any read and removes an
  earlier `(%rbp)` store when a later store overwrites the same slot before any
  read
- rebuilt, ran the filtered `test_x86_peephole_` backend subset plus the new
  direct dead-code test filters, and reran the full ctest suite with monotonic
  results: `181` failures before, `181` failures after, no newly failing tests
- compiled the translated `peephole/passes/copy_propagation.cpp` unit into the
  real x86 backend and test builds
- compiled the translated `peephole/passes/store_forwarding.cpp` unit into the
  real x86 backend and test builds
- wired the translated store-forwarding pass into the live x86 peephole
  optimization loop
- fixed the shared x86 peephole stack-memory classifier so `StoreRbp` and
  `LoadRbp` lines trim register/offset operands correctly before family and
  slot matching
- added a direct regression test that proves the live x86 peephole now drops a
  bounded same-register `(%rbp)` reload after a matching stack store while
  preserving the surrounding control flow
- rebuilt, ran the focused `test_x86_peephole_` backend filter plus the direct
  countdown-loop peephole route test, and reran the full ctest suite with
  monotonic results: `181` failures before, `181` failures after, no newly
  failing tests
- wired the translated register copy-propagation pass into the live x86
  peephole optimization loop ahead of the existing dead-code cleanup
- added a direct regression test that proves the live x86 peephole now
  propagates a transitive register-copy chain into a downstream ALU use and
  lets the existing overwrite-based dead-move cleanup drop the superseded
  copies
- rebuilt, ran the filtered `test_x86_peephole_` backend subset plus the new
  direct copy-propagation test filter, and reran the full ctest suite to the
  same `181`-failure count as the baseline; the stable rerun surfaced the
  existing aggregate `backend_bir_tests` crash path as `SEGFAULT` instead of
  the earlier timeout while keeping the failure set otherwise unchanged
- compiled the translated `peephole/passes/tail_call.cpp` unit into the real
  x86 backend and test builds
- wired the translated tail-call pass into the live x86 peephole optimization
  loop
- added a direct regression test that proves the live x86 peephole now
  rewrites a bounded `call` plus pure epilogue plus `ret` sequence into the
  same epilogue followed by `jmp target`
- rebuilt, ran the focused tail-call regression plus the surrounding
  `test_x86_peephole_` backend subset, reran the intermittent
  `cpp_qualified_template_call_template_arg_perf` case in isolation after a
  transient full-suite failure, and confirmed a second full ctest rerun
  returned to the baseline `181`-failure count with no newly failing tests
