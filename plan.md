# RV64 Call-Adjacent Scalar And Inline-Asm Materialization Plan

Status: Active
Source Idea: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Reactivated After: ideas/closed/432_prepared_inline_asm_operand_home_carriers.md

## Purpose

Finish RV64 object materialization for ordinary scalar call-adjacent and
inline-asm fragments now that prepared scalar GPR inline-asm carrier homes are
coherent.

Goal: coherent scalar call-adjacent and scalar GPR inline-asm rows should no
longer stop at `unsupported_instruction_fragment` in RV64 object lowering.

## Core Rule

Use prepared call and inline-asm carrier metadata as authority. Do not infer
missing homes, argument bindings, return homes, operand relationships,
aggregate/F128 semantics, or memory-constraint behavior from source shape,
representative filenames, asm text, or raw BIR alone.

## Read First

- `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
- `ideas/closed/432_prepared_inline_asm_operand_home_carriers.md`
- `build/agent_state/432_step4_probes/classification.tsv`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- RV64 object materialization for coherent scalar GPR inline-asm carriers that
  still report `unsupported_instruction_fragment`.
- RV64 object-route publication gaps left after prior scalar call-result,
  scalar call-argument, and complete-carrier `.insn r` / `.insn d` work.
- Representative evidence rows: `pr38533`, `pr45695`, and `pr49279`.
- Explicitly out-of-scope representative rows unless a separate idea is
  created: `pr40657` (`=*m` memory constraint) and `pr56982` clobber-only
  `~{memory}` carrier.

## Non-Goals

- Reopening prepared inline-asm producer/carrier home publication closed by
  idea 432 unless fresh evidence proves a producer regression.
- Memory constraints such as `=*m`, address-selection operands, clobber-only
  `~{memory}` carriers without scalar publication, aggregate operands, vector
  operands, and F128 operands.
- Aggregate `sret`/`byval` call-storage lowering, long-double ABI, or F128
  helper-call behavior.
- Reconstructing missing prepared metadata in RV64 object lowering.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- Prior RV64 packets already validated prepared GPR call-result publication,
  existing frame-slot scalar value call-argument support, and complete-carrier
  scalar `.insn r` / `.insn d` inline-asm materialization.
- Idea 432 closed the prepared producer/carrier gap for scalar GPR inline-asm
  homes. Step 4 probes show `missing_operand0_home=no` and
  `tied_input_output_home_mismatch=no` for `pr38533`, `pr45695`, and
  `pr49279`.
- `prepared_function_emit.cpp`, `prepared_call_emit.cpp`, and
  `object_emission.cpp` should consume complete carriers and lower only
  coherent scalar RV64 fragments.
- Remaining object-route failures are RV64 consumer materialization work until
  a fresh probe shows a missing producer fact.

## Execution Rules

- Keep changes semantic and carrier/home driven; do not key behavior to
  `pr38533.c`, `pr45695.c`, `pr49279.c`, or instruction indexes in probe
  dumps.
- Add focused RV64 backend tests before treating representative probe movement
  as progress.
- Preserve prior call-result, call-argument, and complete-carrier `.insn r` /
  `.insn d` behavior.
- Preserve fail-closed diagnostics for unsupported memory/address,
  clobber-only, aggregate, vector, and F128 forms.
- For each code step, run the supervisor-delegated proof command, expected to
  be:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

- Also run `git diff --check` before returning a packet.

## Step 1: Re-Audit Remaining RV64 Object-Route Failures

Goal: identify the first remaining RV64 scalar object-materialization packet
now that producer-side scalar GPR carrier facts are closed.

Actions:

- Inspect `build/agent_state/432_step4_probes/classification.tsv` and the
  representative `.object.log` files for `pr38533`, `pr45695`, and `pr49279`.
- Confirm the next blocker is `unsupported_instruction_fragment` on complete
  scalar GPR carriers, not a renewed `missing_operand0_home` or
  `tied_input_output_home_mismatch`.
- Classify each remaining representative row by RV64 consumer gap: operand
  substitution, result publication, tied input/output movement, asm text
  emission, clobber-only/memory out-of-scope, or another fail-closed reason.
- Record the first coherent implementation packet in `todo.md`.

Completion check:

- `todo.md` names the first RV64 consumer implementation target, primary
  files, representative evidence, out-of-scope rows, and proof command.
- No implementation files are changed in this step.

## Step 2: Materialize Coherent Tied Scalar GPR Inline-Asm Rows

Goal: lower the coherent tied scalar GPR inline-asm forms that now have one
prepared home contract.

Primary targets:

- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Consume prepared inline-asm carriers and tied operand metadata as authority
  for operand substitution and scalar publication.
- Materialize only scalar GPR-compatible tied input/output forms with coherent
  homes.
- Preserve rejection for unsupported constraints, missing carriers, malformed
  operands, memory/address-only shapes, aggregate operands, vector operands,
  and F128 operands.
- Add focused tests that prove the general tied scalar GPR rule without
  keying to representative filenames.

Completion check:

- Focused backend tests prove tied scalar GPR inline-asm materialization.
- The backend subset proof passes and is saved in `test_after.log`.
- Prior complete-carrier `.insn r` / `.insn d` tests still pass.

## Step 3: Materialize Remaining Coherent Scalar Inline-Asm Inputs/Outputs

Goal: generalize the RV64 consumer route for remaining coherent scalar GPR
inline-asm input/output fragments that are not fully covered by Step 2.

Primary targets:

- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Extend scalar inline-asm materialization only for complete prepared carriers
  with coherent scalar GPR operand homes.
- Reuse existing scalar publication helpers where possible instead of adding a
  filename-shaped path.
- Keep memory constraints, clobber-only carriers with no scalar publication,
  aggregate, vector, F128, and missing-prepared-fact shapes fail-closed.
- Add focused tests for representative carrier shapes selected in Step 1.

Completion check:

- Focused backend tests prove the selected coherent scalar inline-asm forms.
- The backend subset proof passes and is saved in `test_after.log`.
- Unsupported and out-of-scope rows still reject cleanly.

## Step 4: Re-Probe Representatives And Decide Close Readiness

Goal: determine whether the RV64 call-adjacent scalar and inline-asm source
idea is complete or whether remaining failures need a separate follow-up idea.

Actions:

- Re-run representative RV64 object-route probes for `pr38533`, `pr45695`,
  and `pr49279`.
- Confirm that remaining failures, if any, are either outside this source idea
  or have a new durable follow-up idea under `ideas/open/`.
- Keep `pr40657` memory-constraint and `pr56982` clobber-only carrier work
  outside this scalar GPR object-materialization plan unless earlier steps
  prove a shared RV64 consumer invariant requires them.
- Run the supervisor-delegated backend proof and `git diff --check`.
- Summarize acceptance coverage, rejected row classes, and close readiness in
  `todo.md`.

Completion check:

- `todo.md` records whether the source idea appears ready for lifecycle close
  review.
- No expectation, allowlist, unsupported-marker, runtime-comparison, or
  pass/fail accounting files are changed or used as progress.
