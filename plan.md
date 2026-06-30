# Prepared Inline-Asm Operand Home Carriers Plan

Status: Active
Source Idea: ideas/open/432_prepared_inline_asm_operand_home_carriers.md
Supersedes Active Runbook: `RV64 Call-Adjacent Scalar And Inline-Asm Materialization Plan`

## Purpose

Repair prepared inline-asm producer/carrier facts that block scalar GPR
inline-asm rows after RV64 object lowering has already proven its
complete-carrier consumer path.

Goal: supported scalar GPR inline-asm inputs, outputs, and tied operands should
reach target lowering with complete, coherent prepared operand homes.

## Core Rule

Prepared carriers own inline-asm operand authority. Do not make RV64 object
lowering infer missing operand homes, tied-home relationships, or unsupported
memory/address behavior from source shape, representative filenames, asm text,
or raw BIR.

## Read First

- `ideas/open/432_prepared_inline_asm_operand_home_carriers.md`
- `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
- `build/agent_state/428_step5_probe/probe_summary.tsv`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prepared_printer/inline_asm.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`
- `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Prepared inline-asm carrier operand homes for scalar GPR register outputs.
- Prepared inline-asm carrier operand homes for scalar GPR register inputs.
- Tied scalar GPR input/output home coherence for `=r,0`-style constraints.
- Representative blocker facts: `missing_operand0_home` for `pr38533` and
  `tied_input_output_home_mismatch` for `pr45695` / `pr49279`.

## Non-Goals

- RV64 target reconstruction of missing prepared inline-asm homes.
- Memory constraints such as `=*m`, address-selection operands, clobber-only
  `~{memory}` carriers, aggregate operands, vector operands, and F128 operands.
- Aggregate ABI, pointer/address publication, F128 helper-call behavior, or
  broad object-emission rewrites.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- `src/backend/bir/lir_to_bir/calling.cpp` parses inline-asm constraints into
  BIR metadata, including register inputs, register outputs, tied inputs,
  memory/address operands, and clobbers.
- Prepared inline-asm carrier construction and printing expose whether target
  consumers receive complete operand homes or a producer-owned missing fact.
- RV64 object lowering should consume only complete carriers; missing operand
  homes and tied-home contradictions remain producer/carrier blockers until
  this plan repairs or precisely rejects them.

## Execution Rules

- Keep changes semantic and operand/home driven; do not key behavior to
  `pr38533.c`, `pr45695.c`, `pr49279.c`, or instruction indexes in probe
  dumps.
- Add focused prepared-contract tests before treating representative probe
  movement as progress.
- Preserve complete-carrier `.insn r` / `.insn d` behavior.
- Preserve fail-closed diagnostics for unsupported memory/address,
  clobber-only, aggregate, vector, and F128 forms.
- For each code step, run the supervisor-delegated proof command, expected to
  be:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

- Also run `git diff --check` before returning a packet.

## Step 1: Audit Inline-Asm Carrier Producer State

Goal: identify the exact producer/carrier surface that emits
`missing_operand0_home` and `tied_input_output_home_mismatch`.

Actions:

- Inspect the Step 5 probe artifacts for `pr38533`, `pr45695`, and `pr49279`.
- Trace the path from LIR inline-asm constraints through BIR inline-asm
  metadata and prepared carrier construction.
- Classify each missing fact as missing source value home, missing result home,
  tied operand reconciliation, unsupported operand kind, or printer-only
  evidence.
- Record the first implementation packet in `todo.md`.

Completion check:

- `todo.md` names the first producer/carrier implementation target, the exact
  old missing fact it should eliminate, and the proof command.
- No implementation files are changed in this step.

## Step 2: Publish Scalar GPR Operand Homes

Goal: ensure supported scalar GPR inline-asm register inputs and outputs have
complete prepared operand homes.

Primary targets:

- `src/backend/bir/lir_to_bir/calling.cpp`
- prepared carrier construction and lookup surfaces identified in Step 1
- focused prepared-contract tests under `tests/backend/bir/`

Actions:

- Repair producer logic for scalar GPR register input/output operands whose
  source values and result homes are already known.
- Keep unsupported memory/address, clobber-only, aggregate, vector, and F128
  operands fail-closed.
- Add tests that assert complete carrier facts rather than relying on target
  object output.

Completion check:

- Focused tests prove scalar GPR input/output operand homes are present.
- The backend subset proof passes and is saved in `test_after.log`.
- Existing complete-carrier RV64 inline-asm tests still pass.

## Step 3: Reconcile Tied Scalar GPR Operand Homes

Goal: make supported tied scalar GPR input/output operands publish one coherent
home contract.

Primary targets:

- tied operand handling identified in Step 1
- focused prepared-contract tests under `tests/backend/bir/`

Actions:

- Handle `=r,0`-style tied operands by tying the input home to the output home
  only when both sides are scalar GPR-compatible and semantically the same
  operand contract.
- Reject mismatches with a precise producer-owned diagnostic when the homes
  cannot be reconciled.
- Avoid changes to RV64 object lowering for this packet.

Completion check:

- Focused tests prove tied scalar GPR operand home coherence.
- The backend subset proof passes and is saved in `test_after.log`.
- Unsupported or incoherent tied operands still reject cleanly.

## Step 4: Re-Probe Representative Rows And Decide Close Readiness

Goal: determine whether the prepared producer/carrier blockers are resolved
and whether the parked RV64 call-adjacent idea can be reconsidered.

Actions:

- Re-run prepared and RV64 object-route probes for `pr38533`, `pr45695`, and
  `pr49279`.
- Confirm that `missing_operand0_home` and
  `tied_input_output_home_mismatch` are gone or replaced by a more precise
  producer-owned unsupported fact.
- Keep `pr40657` memory-constraint and `pr56982` clobber-only carrier work
  classified outside this scalar GPR plan unless earlier steps prove a shared
  carrier invariant requires them.
- Run the supervisor-delegated backend proof and `git diff --check`.
- Summarize whether this idea is ready for lifecycle close and whether
  `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
  can be reactivated or closed.

Completion check:

- `todo.md` records close readiness for this idea and the disposition of the
  parked RV64 call-adjacent idea.
- No expectation, allowlist, unsupported-marker, runtime-comparison, or
  pass/fail accounting files are changed or used as progress.
