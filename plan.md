# RV64 Runtime External Abort Exit Support Plan

Status: Active
Source Idea: ideas/open/437_rv64_runtime_external_abort_exit_support.md

## Purpose

Resolve fail-closed RV64 codegen residuals for ordinary runtime external calls
such as `abort` and `exit` after unrelated aggregate `sret` call-storage
blockers have been cleared.

## Goal

Classify and implement the narrow RV64 policy for accepted runtime external
termination calls, while preserving explicit fail-closed diagnostics for
unsupported runtime externals.

## Core Rule

Runtime external support must be semantic and policy-backed. Do not special-case
representative filenames, caller names, block indexes, or one dump shape, and do
not weaken unsupported/runtime-comparison behavior to claim progress.

## Read First

- `ideas/open/437_rv64_runtime_external_abort_exit_support.md`
- `ideas/closed/435_rv64_coherent_aggregate_sret_call_storage_lowering.md`
- `build/agent_state/435_step3_sret_memory_return/summary.txt`
- `build/agent_state/435_step3_sret_memory_return/20000917-1.dump_prepared_bir.out`
- `build/agent_state/435_step3_sret_memory_return/20020206-1.dump_prepared_bir.out`
- `build/agent_state/435_step3_sret_memory_return/20000917-1.codegen_asm.err`
- `build/agent_state/435_step3_sret_memory_return/20020206-1.codegen_asm.err`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- `unsupported_external_call` for direct fixed-arity runtime external `abort`
  after aggregate `sret` facts are cleared:
  - `src/20000917-1.c`: function `main`, callee `abort`
  - `src/20020206-1.c`: function `baz`, callee `abort`
- Closely related `exit` declaration/call shapes visible in the same prepared
  dumps.
- RV64 policy for whether `abort`/`exit` lower through ordinary external call
  emission, a runtime support shim, or a deliberate fail-closed route.

## Non-Goals

- Aggregate `sret`/`byval` lowering.
- Runtime mismatch triage for programs that already compile, link, and run but
  produce wrong results.
- F128 helper policy, softfloat runtime helpers, inline asm, select lowering,
  local-publication, post-call aggregate copy, or pointer residuals.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- The representatives now reach runtime external calls only after aggregate
  `sret` call-storage is no longer the first owner.
- Prepared BIR records both `abort` and `exit` declarations and direct
  fixed-arity callsites.
- `abort` is currently the first RV64 asm residual; `exit` appears nearby and
  should be classified under the same runtime external policy before any
  implementation packet broadens support.

## Execution Rules

- Start with classification and a focused test target; do not implement broad
  runtime-helper infrastructure first.
- Preserve fail-closed diagnostics for unsupported runtime externals outside
  the accepted policy.
- If the policy requires linking/runtime support rather than codegen-only
  emission, record that boundary in `todo.md` before implementation.
- Any later select, local-publication, post-call copy, pointer, or aggregate
  residual exposed after runtime external support must be routed separately.
- For code/test packets, use:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Steps

### Step 1: Classify Runtime External Policy And First Packet

Inspect current RV64 direct external call emission and the representative
prepared call facts for `abort` and `exit`. Decide whether the first packet is
diagnostic-only policy, ordinary external call emission, runtime shim support,
or a narrower fail-closed contract.

Completion signal: `todo.md` names the accepted runtime external policy, the
first implementation/test packet, and any shapes that must remain unsupported.

### Step 2: Add Focused Runtime External Coverage

Add backend coverage for the accepted `abort`/`exit` policy and for at least one
rejected unsupported runtime external shape if the policy is not fully general.

Completion signal: focused tests prove semantic runtime external behavior
without relying on representative testcase names.

### Step 3: Implement Bounded Runtime External Support

Implement the chosen RV64 runtime external support or fail-closed classifier.
Keep the change scoped to the accepted policy and preserve unrelated call,
helper, and aggregate behavior.

Completion signal: backend proof passes, and representative probes no longer
stop at `unsupported_external_call` for the accepted `abort`/`exit` shape.

### Step 4: Residual Disposition And Close Readiness

Classify any residual exposed after runtime external support. Route non-runtime
residuals to existing or new ideas rather than broadening this source idea.

Completion signal: lifecycle review can close this idea, or keep it active
with an exact remaining runtime external packet.
