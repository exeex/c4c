# Current Packet

Status: Active
Source Idea Path: ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Pin First Downstream Unsupported Fragment

## Just Finished

Step 1 (`Reconstruct Div/Rem Lowering Boundary`) completed the evidence
packet for the routed integer div/rem rows.

- Authoritative row artifact:
  `build/agent_state/567_step1_div_rem_rows.tsv`
- Row count: `30` data rows plus header, extracted from the refreshed coherent
  Step 4 screening table where `step3_family == integer_div_rem`.
- Representative allowlist:
  `build/agent_state/567_step1_div_rem_representative.allowlist`
- Representative log:
  `build/agent_state/567_step1_div_rem_representative.log`
- Representative result: `total=5 passed=0 failed=5`.
- Representative diagnostics:
  - `src/20001026-1.c`: `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
  - `src/20050215-1.c`: `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
  - `src/20090113-2.c`: `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
  - `src/20090113-3.c`: `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
  - `src/20101013-1.c`: `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
- Representative dumps:
  - `build/agent_state/567_step1_20001026-1.bir.txt`
  - `build/agent_state/567_step1_20001026-1.prepared_bir.txt`

Hook findings:

- `src/backend/mir/riscv/codegen/object_emission.cpp:7413` already routes
  `bir::BinaryInst` through `fragment_for_prepared_binary(...)`; failure falls
  through to the generic diagnostic at
  `src/backend/mir/riscv/codegen/object_emission.cpp:8137`.
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp:2037` is the concrete
  prepared object-emission hook for semantic binary lowering.
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp:2321` through
  `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp:2347` already encodes
  `SDiv`, `UDiv`, `SRem`, and `URem` with RV64 M-extension `div/divu/rem/remu`
  and `divw/divuw/remw/remuw` opcode selection based on I32 versus I64 result
  type.
- `src/backend/mir/riscv/codegen/alu.cpp:16` through
  `src/backend/mir/riscv/codegen/alu.cpp:19` also already has scalar ALU
  mnemonic selection for `div/divu/rem/remu` and `*w` forms.
- Focused object-emission coverage already exists at
  `tests/backend/mir/backend_riscv_object_emission_test.cpp:13594` and covers
  all four div/rem opcodes at I32 and I64 width.
- The `src/20001026-1.c` dumps confirm coherent semantic/prepared
  `bir.udiv i64 24, 8`; the prepared home for `%t1` is a GPR stack slot, so
  the next packet should first identify the exact later `BinaryInst`/`SelectInst`
  or publication shape still falling through the generic diagnostic before
  adding any new div/rem lowering.

## Suggested Next

Plan-owner repair advanced the active route to Step 2:
`Pin First Downstream Unsupported Fragment`.

Executor packet for Step 2: add focused instrumentation or a targeted
unit-level reproducer that pins the first unsupported instruction in one routed
representative after the existing div/rem fragment path. Do not add duplicate
div/rem opcode lowering or duplicate all-opcode div/rem tests. If the pinned
fragment is implementation-ready RV64 object lowering, record the exact repair
target for Step 3; otherwise route the row to a concrete downstream owner in
`todo.md`.

## Watchouts

- Use the refreshed coherent 2026-07-03 row artifacts, not the stale 137-row
  or mixed-time 179-row evidence from the earlier classification run.
- Keep screened-out F128, producer/prepared, ABI/call, evidence-gap,
  shift-right, pointer-cast, scalar-FP, and heterogeneous scalar-integer rows
  out of this implementation plan.
- Reject testcase-name dispatch, opcode-text-only matching, expectation
  rewrites, unsupported downgrades, and allowlist-only progress.
- The current tree already has semantic div/rem opcode encoding and focused
  div/rem object tests; the representative failures may be later same-diagnostic
  fragments in the routed rows rather than missing raw `div/rem` encoders.
- Leave `review/557_step13_vector_local_memory_review.md` untouched.

## Proof

- Baseline backend CTest was already refreshed by the supervisor before this
  evidence packet; no additional CTest proof was required.
- Representative command:
  `ALLOWLIST=build/agent_state/567_step1_div_rem_representative.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/567_step1_div_rem_representative.log 2>&1`
- Representative command result: exit `1`, expected for current failing rows.
- Local validation: `git diff --check -- todo.md`
