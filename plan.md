# RV64 Pointer Cast And Address Materialization Plan

Status: Active
Source Idea: ideas/open/429_rv64_pointer_address_materialization.md

## Purpose

Lower coherent RV64 pointer cast and address materialization fragments while
keeping pointer provenance, object identity, relocation, and addressability
producer gaps out of target lowering.

Goal: prepared pointer/address rows with coherent facts should no longer stop
at `unsupported_instruction_fragment`.

## Core Rule

Use prepared pointer, address, relocation, and object facts as authority. Do
not infer object identity, provenance, relocation base, stack/global
addressability, or pointer meaning from integer shape, source filename, raw
BIR, or representative row text.

## Read First

- `ideas/open/429_rv64_pointer_address_materialization.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Current Targets

- Coherent prepared `inttoptr` and `ptrtoint` fragments.
- Address-bearing scalar rows whose provenance and object facts are explicit
  enough for RV64 target lowering.
- Representative rows: `src/930930-1.c`, `src/20000622-1.c`,
  `src/20010329-1.c`, `src/20011019-1.c`, and `src/20041112-1.c`.

## Non-Goals

- Guessing pointer provenance, object identity, relocation base, or
  addressability in RV64 lowering.
- Repairing BIR or prepared pointer provenance producers.
- Absorbing global memory/addressing residual rows that require separate
  addressability review.
- Aggregate ABI, call publication, select/join, F128, scalar floating-point,
  integer div/rem, or broad memory-lowering work.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- Pointer/address lowering is valid only when the prepared module carries
  enough semantic authority for the target to move or materialize the value.
- Register-sized `ptrtoint` and `inttoptr` movement may be RV64 scalar
  materialization when the source and destination homes are coherent.
- Address materialization that depends on object identity, relocation, frame
  slot, global symbol, or pointer-base facts must consume prepared facts or
  stay fail-closed.
- If a row exposes missing or incoherent provenance, route that fact to a
  producer-owned follow-up instead of patching around it in RV64.

## Execution Rules

- Keep changes semantic and fact-driven; do not key behavior to
  `930930-1.c`, `20000622-1.c`, `20010329-1.c`, or any other representative
  filename.
- Add focused backend tests for each newly supported pointer/address shape.
- Preserve fail-closed diagnostics for missing provenance, object identity,
  relocation, addressability, operand/result home, or unsupported width facts.
- If execution discovers a producer gap, record it in `todo.md` and request a
  lifecycle split instead of expanding this target-lowering plan.
- For each code step, run the supervisor-delegated proof command, expected to
  be:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

- Also run `git diff --check` before returning a packet.

## Step 1: Classify Pointer/Address Evidence And Choose First Packet

Goal: identify the first narrow pointer/address implementation packet that has
coherent prepared facts.

Actions:

- Inspect the pointer/address representative rows and available handoff
  artifacts for the 21-row bucket.
- Classify rows into coherent `inttoptr`, coherent `ptrtoint`, address-bearing
  scalar materialization, global/addressability residual, and
  missing-provenance producer-gap buckets.
- Confirm the first implementation packet has explicit prepared source and
  destination authority plus a focused backend test shape.
- Record the chosen packet, rejected row classes, primary files, and proof
  command in `todo.md`.

Completion check:

- `todo.md` names the first implementation target, representative evidence,
  out-of-scope rows, and exact proof command.
- No implementation files are changed in this step.

## Step 2: Lower Coherent Pointer Cast Movement

Goal: materialize prepared `inttoptr` and `ptrtoint` fragments when the cast is
register-sized scalar movement with coherent homes.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Reuse existing scalar register/publication helpers where the cast is a
  bit-preserving pointer-width move.
- Require prepared operand and result homes; reject missing or incoherent
  provenance instead of fabricating it.
- Keep unsupported widths, aggregate objects, F128, and memory/address-only
  shapes fail-closed.
- Add focused tests for both `inttoptr` and `ptrtoint` coherent facts.

Completion check:

- Focused backend tests prove coherent pointer cast materialization.
- The backend subset proof passes and is saved in `test_after.log`.
- Missing-provenance rows still reject cleanly.

## Step 3: Lower Coherent Address Materialization

Goal: lower address-bearing scalar rows only when prepared object or address
facts give RV64 enough authority to emit the address.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Consume prepared frame-slot, pointer-base, symbol, or relocation facts only
  when they are explicit and coherent.
- Keep global memory/addressing residual rows separate unless the facts prove
  they are ordinary address materialization within this source idea.
- Preserve fail-closed diagnostics for incomplete object identity,
  relocation, addressability, or pointer-base facts.
- Add focused tests for each address materialization form accepted by this
  step.

Completion check:

- Focused backend tests prove the selected address materialization forms.
- The backend subset proof passes and is saved in `test_after.log`.
- Producer gaps are recorded for lifecycle split instead of hidden in target
  code.

## Step 4: Re-Probe Representatives And Decide Close Readiness

Goal: decide whether the pointer/address source idea is complete or whether
remaining rows require separate producer or global-addressing ideas.

Actions:

- Re-run representative RV64 object-route probes for the rows selected in
  Step 1.
- Classify any remaining failures as in-scope pointer/address target work or
  out-of-scope producer provenance, global memory/addressing, aggregate, F128,
  select/join, call, or accounting work.
- Run the supervisor-delegated backend proof and `git diff --check`.
- Summarize acceptance coverage, rejected row classes, and close readiness in
  `todo.md`.

Completion check:

- `todo.md` records whether the source idea appears ready for lifecycle close
  review.
- No expectation, allowlist, unsupported-marker, runtime-comparison, or
  pass/fail accounting files are changed or used as progress.
