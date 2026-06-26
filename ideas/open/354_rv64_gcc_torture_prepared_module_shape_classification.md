# RV64 gcc_torture Prepared Module Shape Classification

Status: Open
Type: Umbrella analysis idea

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
