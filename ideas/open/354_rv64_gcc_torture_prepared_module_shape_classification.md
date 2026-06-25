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
