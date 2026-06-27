# RV64 gcc_torture Prepared Module Shape Classification

Status: Closed
Type: Umbrella analysis idea
Closed By: Final child-coverage audit and classification closeout
Reopened: 2026-06-26 by user request after fresh RV64 gcc_torture backend scan
showed `211` pass / `1256` fail across `1467` selected cases.

## Context

The RV64 gcc torture backend execution scan is now available through:

```sh
scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

The first full scan over `tests/c/external/gcc_torture/allowlist.txt` produced:

- `1467` selected cases
- `3` passing cases
- `1464` failing cases
- `1012` failures containing:
  `RISC-V backend object route unsupported prepared module shape`

The scan already proves that the runner can reach `c4cll --codegen obj`, clang
RV64 link, and qemu-riscv64 execution. The pass cases had no runtime mismatch.
The dominant blocker is therefore an object-route admission/classification
problem before many cases reach meaningful RV64 object emission.

## Goal

Classify the `prepared module shape` failure family into actionable backend
capability groups and produce follow-up repair ideas for the real blocking
families.

This is an analysis and planning umbrella. It should not implement broad
lowering repairs directly.

## Inputs

- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` when available
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` when available
- per-case logs under `build/rv64_gcc_c_torture_backend/` when available
- `tests/c/external/gcc_torture/allowlist.txt`
- RV64 object-route diagnostics in the backend code

If the build artifacts are absent, rerun a representative scan:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Use `MAX_CASES=<n>` or a temporary copied allowlist only for probes; the final
classification must be grounded in the full allowlist or an explicitly
documented sampled subset.

## Required Analysis

1. Reproduce or load the RV64 gcc_torture backend scan results.
2. Extract every case whose log contains
   `RISC-V backend object route unsupported prepared module shape`.
3. For those cases, inspect nearby diagnostic state and source features to
   classify the prepared module shapes into stable semantic buckets.
4. Identify which buckets are real backend gaps, which are frontend/GNU C
   extension gaps, and which are diagnostic/admission-policy ambiguity.
5. Pick at least one minimal representative case for each major bucket.
6. For every repairable backend bucket, create a follow-up idea under
   `ideas/open/` with:
   - the representative cases
   - the expected backend stage to repair
   - the proof command using the RV64 gcc_torture backend scan or a narrow
     per-case runner invocation
   - explicit non-overfit constraints

## Non-Goals

- Do not mark gcc_torture cases unsupported to make the scan greener.
- Do not weaken the RV64 backend object runner.
- Do not implement testcase-shaped shortcuts.
- Do not fold semantic `lir_to_bir` family fixes into this umbrella unless the
  prepared-shape classification proves they are the same root blocker.
- Do not require this heavy scan in ordinary full CTest.

## Acceptance

This umbrella may close only when all of these are true:

- The `prepared module shape` failures are classified from actual scan data.
- The classification includes counts and representative cases.
- Repairable backend buckets have child ideas in `ideas/open/`.
- Non-backend or not-yet-actionable buckets are explicitly documented with the
  reason they are not being repaired first.
- All generated child ideas have been closed or intentionally superseded.
- A final review confirms that the child idea set covers the dominant prepared
  module shape failures and does not overfit individual torture cases.

## Suggested First Step

Start by writing a small local classifier or one-off command sequence that maps
case logs to:

- `prepared-module-shape`
- `semantic-lir-to-bir`
- `bootstrap-lowering-only-supports`
- `timeout`
- `runtime-mismatch`
- `link-or-qemu`

Then drill into the `prepared-module-shape` subset and split it further by the
prepared BIR/MIR/module feature that caused object-route rejection.

## Analysis Handoff

Classification artifact:

- `review/354_prepared_shape_classification.md`

The `1012` prepared-shape failures were classified from existing scan logs and
per-case `--dump-prepared-bir` output. Primary buckets:

- `540` multi-block control flow
- `378` globals or non-string global addresses
- `70` string constants / string data addresses
- `11` general call lowering shape
- `6` non-i32 or pointer local memory width
- `3` variadic or vararg entry/call shape
- `2` declaration control-flow entries
- `1` floating-point or FPR ABI value
- `1` local memory addressing/home shape

Architecture-level conclusion: the RV64 object route currently has a narrow
direct prepared-object encoder while a broader prepared RV64 asm emitter exists
separately. Repair work should avoid growing two independent lowering surfaces.

Generated child ideas:

- `ideas/open/355_rv64_prepared_object_shape_diagnostics.md`
- `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`
- `ideas/open/357_rv64_object_route_data_sections_globals_strings.md`
- `ideas/open/358_rv64_object_route_abi_width_edges.md`

## Layer-Routing Update After 359

Idea 359 (`ideas/open/359_bir_prepared_object_consumer_contract_completion.md`)
is now the active upstream prerequisite for the prepared-object consumer
contract. It pauses direct execution of 356 until the shared BIR/prepared
contract is clear enough that target object emission does not absorb CFG,
publication, select-carrier, value-home, or frame ownership semantics.

The original child split should be read with explicit layer boundaries:

- 355 owns diagnostics layering. Shared prepared-object rejection taxonomy and
  consumer-contract diagnostics belong with 359; RV64 diagnostics should only
  consume that taxonomy and append target-specific evidence.
- 356 remains the RV64 object-route architecture continuation after 359. It
  must not make MIR/assembler/object emission reconstruct BIR-owned CFG or
  data-flow semantics.
- 357 owns RV64 ELF data sections, symbols, and relocations only for data that
  is already represented by the prepared module contract. Missing
  string/global/initializer representation is upstream BIR/LIR work, not RV64
  object-emission work.
- 358 is a mixed edge bucket and must route each edge by layer. Vararg/va_arg
  belongs to a separate LIR/BIR contract idea, while local memory homes,
  declaration-control-flow entries, and value-home width edges depend on 359
  before target emission repairs continue.

Additional upstream child ideas created by this layer split are part of this
umbrella's closure condition. This umbrella should not close until those
upstream children are either closed or intentionally superseded, in addition to
the original 355/356/357/358 child set.

## Step 4 Closure Decision

Closure is deferred after the Step 3 representative refresh on 2026-06-25.
The original generated child chain through 367 is closed, and the refresh no
longer shows opaque unclassified prepared-module-shape failures, but it does
show residual structured RV64 object-route buckets without open lifecycle
owners.

Step 3 evidence:

- `build/agent_state/354_step3_representative_refresh.log`
- `total=18 passed=11 failed=7`

New follow-up child ideas:

- `ideas/closed/368_rv64_object_route_frame_slot_base_offset_memory.md`
- `ideas/open/369_rv64_object_route_terminator_fragment_lowering.md`
- `ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md`
- `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`
- `ideas/closed/372_rv64_object_route_frame_slot_address_call_args.md`
- `ideas/closed/373_rv64_object_route_frame_slot_value_call_args.md`
- `ideas/open/374_rv64_object_route_non_register_param_homes.md`
- `ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md`
- `ideas/closed/383_rv64_global_aggregate_lane_materialization.md`
- `ideas/closed/384_prepared_global_symbol_memory_access_publication.md`
- `ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md`

These follow-ups intentionally keep the residual buckets outside this
classification umbrella's implementation scope. Idea 354 remains open until
the new follow-up children are closed or intentionally superseded and a final
closure review confirms the child set covers the dominant prepared-module-shape
failures without testcase overfit.

## Follow-up After 384

Idea 384 closed the prepared global-symbol publication boundary for
`src/20030914-2.c`. The representative now reaches a same-module call with a
byval aggregate/address argument:

```text
%t1 = bir.call i32 f(ptr byval(size=72, align=4) %t0, i32 4660)
```

That boundary is split into child idea
`ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md`.
It is distinct from closed byval parameter-home idea 370 because the current
failure is call argument lowering, not entry parameter-home admission.

## Historical Closeout Notes Before Reopen

These notes record the earlier 354 closeout before the 2026-06-26 reopen. They
are preserved as historical evidence, but the final lifecycle decision is the
later `Final Reopen Closeout - 2026-06-27` section.

At that earlier checkpoint, the original prepared-module-shape failures were
classified from actual scan data in
`review/354_prepared_shape_classification.md`, including counts and
representative cases for the original `1012` opaque prepared-module-shape
failures. Non-backend or not-yet-actionable families were explicitly separated
from repairable RV64 object-route buckets.

Historical child and residual owner audit at that checkpoint:

- original child set 355-359: closed
- upstream/continuation chain 360-367: closed
- representative-refresh residual children 368-375: closed
- additional residual owner IDs 376-382: closed
- global/data and same-module byval follow-ups 383, 384, and 386: closed
- later representative/runtime owner chain 387-394: closed

Idea 385 was unrelated EV64 `.insn.d` length-prefix work, not a child or
blocker for this RV64 gcc_torture prepared-module-shape umbrella. It is also
closed in the final repository state.

The historical closeout evidence reviewed:

- `review/354_prepared_shape_classification.md`
- `build/agent_state/354_step3_representative_refresh.log`

The earlier closeout used then-current scan artifacts. In the final 2026-06-27
closure state, `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` is
only a one-case `src/int-compare.c` representative artifact and is not the
basis for final closure.

The historical plan-owner close gate passed with 326/326 backend tests and no
new failures.

## Reopen Classification Pass - 2026-06-26

Classification artifact:

- `review/354_reopen_classification_20260626.md`

The reopened pass used the latest existing scan artifacts rather than rerunning
the full 1467-case scan:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- per-case logs under `build/rv64_gcc_c_torture_backend/`

Current scan status:

- total: `1467`
- pass: `211`
- fail: `1256`

Top-level failure split:

- `1214` `RV64_C4C_OBJ_COMPILE_FAIL`
- `34` `RV64_BACKEND_RUNTIME_MISMATCH`
- `8` `RV64_C4C_OBJ_COMPILE_TIMEOUT`

The compile failures split into `770` current RV64 prepared-object
lowering/admission failures and `444` semantic `lir_to_bir` handoff failures.
The RV64 prepared-object failures were converted into seven new repair ideas:

- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
  (`385` failures)
- `ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md`
  (`88` failures)
- `ideas/open/397_rv64_object_route_move_bundle_target_shapes.md`
  (`92` failures)
- `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`
  (`103` failures)
- `ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`
  (`30` failures)
- `ideas/open/400_rv64_object_route_local_memory_addressing_edges.md`
  (`30` failures)
- `ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md`
  (`42` failures)

The runtime mismatch bucket was converted into one new repair/classification
idea:

- `ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md`
  (`34` failures)

Buckets intentionally not converted to RV64 backend follow-up ideas in this
pass:

- `444` semantic `lir_to_bir` handoff failures. They fail before prepared
  object handoff and need a separate semantic owner if pursued.
- `8` compile timeouts. The current logs prove a timeout symptom but not yet
  whether the owner is RV64 backend, semantic lowering, or an earlier compiler
  stage.

Lifecycle decision: 354 remains open and incomplete after this pass. It
produced `8` new follow-up ideas, and those children must be closed or
intentionally superseded before this umbrella can close again.

## Final Reopen Closeout - 2026-06-27

The reopened umbrella is closed after Step 1 and Step 2 lifecycle audits
confirmed the generated child and residual owner chain is complete.

Closure evidence:

- Step 1 audit artifact
  `build/agent_state/354_step1_child_closure_audit/child_closure_audit.txt`
  showed `ideas/open/` contains only 354 and ideas 395 through 411 are all
  archived under `ideas/closed/` with `Status: Closed`.
- A supervisor cross-check confirmed ideas 355 through 394 are also under
  `ideas/closed/`.
- Step 2 audit artifact
  `build/agent_state/354_step2_residual_coverage/residual_coverage_audit.txt`
  reviewed the original classification in
  `review/354_prepared_shape_classification.md` and the reopened
  classification in `review/354_reopen_classification_20260626.md`.
- The reopened classification converted `770` RV64 prepared-object route
  diagnostics to ideas 395 through 401 and `34` runtime mismatches to idea
  402; their split follow-ups through 411 are closed.
- The `444` semantic `lir_to_bir` handoff diagnostics are documented as
  outside this RV64 prepared-shape umbrella unless pursued by separate
  semantic ownership.
- The `8` timeout diagnostics remain documented as a symptom bucket not
  assigned to RV64 backend versus semantic/frontend ownership from the
  available logs.
- The current
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` is only a
  one-case `src/int-compare.c` representative artifact, so this closeout
  intentionally relies on the saved review artifacts plus the closed child
  chain audit rather than treating that file as a current full scan.
- No concrete unowned residual owner was found.

Close gate:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.
