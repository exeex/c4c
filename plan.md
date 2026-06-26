# RV64 Object Route Instruction Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Activated after: ideas/closed/403_prepared_i16_formal_abi_publication.md

## Purpose

Repair or retire the RV64 prepared-object route bucket where prepared BIR
instructions reach object emission but are rejected as unsupported instruction
fragments.

## Goal

Refresh the current instruction-fragment bucket after the producer-side child
repairs, then lower any remaining reusable RV64 instruction-fragment family or
prepare 395 for lifecycle closure if the source acceptance is already met.

## Core Rule

RV64 object emission may lower explicit prepared instruction facts. It must not
reconstruct missing producer ABI facts, BIR control/data-flow semantics, source
testcase intent, or filename-specific behavior.

## Read First

- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/403_prepared_i16_formal_abi_publication.md`
- `ideas/closed/407_prepared_i16_same_module_call_arg_abi_publication.md`
- `tests/c/external/gcc_torture/src/20000223-1.c`
- `tests/c/external/gcc_torture/src/divmod-1.c`
- Current RV64 gcc_torture backend probe artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Primary source-idea representative:
  `src/20000223-1.c`, the original dominant-bucket representative for
  `unsupported_instruction_fragment`.
- Recently routed representative:
  `src/divmod-1.c`, after corrected 407 and reclosed 403; 403 Step 3 recorded
  `c4cll_status=0` and no fresh residual.
- Nearby same-fragment cases selected by the supervisor from current RV64
  gcc_torture backend artifacts.

## Non-Goals

- Do not reopen closed idea 403 unless fresh prepared dumps again show direct
  `I16` formals with `encoding=register bank=none reg=aN`.
- Do not reopen closed idea 407 unless fresh prepared dumps again show the old
  same-module `I16` caller-side producer regression:
  `source_encoding=frame_slot ... dest_bank=none`,
  `call_arg_stack_to_stack`, or `placement=none:call_argument`.
- Do not infer scalar ABI policy, parameter homes, or register banks in
  `src/backend/riscv/rv64/object_emission.cpp`.
- Do not absorb terminator lowering, move-bundle target shapes, stack-frame
  admission, parameter-home, global data, or runtime-mismatch work owned by
  separate open ideas.
- Do not rewrite gcc_torture expectations, mark cases unsupported, weaken
  allowlists, or special-case testcase filenames.

## Working Model

395 was interrupted by producer-side child routes. Corrected 407 repaired
same-module `I16` call-argument destination facts. Reclosed 403 repaired direct
`I16` formal GPR bank publication and Step 3 proved `src/divmod-1.c` completes
with `c4cll_status=0`, without routing a new residual back to 395.

The next packet should refresh the current unsupported-instruction-fragment
bucket before implementation. If no owned 395 representative still fails with
`unsupported_instruction_fragment`, the executor should record closure evidence
for plan-owner review instead of inventing an opcode-lowering packet.

## Execution Rules

- Start with classification proof, not implementation, because child producer
  routes may have cleared the current representatives.
- Keep each implementation packet tied to one concrete prepared
  instruction-fragment family.
- Inspect prepared dumps and object-route diagnostics before editing target
  emission.
- Prefer semantic RV64 lowering that applies to a same-fragment family.
- If a representative lacks required prepared facts, stop and route that
  producer boundary instead of compensating in the RV64 emitter.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- If a repaired case object-compiles and links, include qemu comparison in the
  proof.
- Treat diagnostic-only churn, expectation rewrites, allowlist-only progress,
  and named-case green proof as route failures.

## Step 1: Refresh Instruction-Fragment Bucket After Child Closures

Goal: determine whether 395 still has an owned `unsupported_instruction_fragment`
representative after corrected 407 and reclosed 403.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `src/20000223-1.c`, `src/divmod-1.c`, and any nearby same-fragment cases.
- Dump or inspect prepared BIR for any case that still fails with
  `unsupported_instruction_fragment`.
- Confirm closed 403 and 407 producer facts do not regress before assigning a
  residual to 395.
- If representatives pass or route elsewhere, record closure/deactivation
  evidence rather than forcing an implementation packet.
- If an owned residual remains, name the exact instruction opcode, operand
  facts, value banks, placements, and source/dest facts that reach object
  emission.

Completion check:

- `todo.md` records either closure-ready evidence for 395 or the concrete
  instruction-fragment family for the first executor packet, with the exact
  supervisor-delegated proof command.
- Any non-395 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Lower The First Valid Instruction-Fragment Family

Goal: add reusable RV64 object lowering for the first classified prepared
instruction-fragment family when all required prepared facts are explicit.

Actions:

- Update the RV64 object route to consume the prepared instruction facts for
  the selected family.
- Preserve existing diagnostics for unsupported or incomplete prepared facts.
- Add or update focused backend coverage where the repo has a matching test
  surface for the instruction family.
- Keep changes narrow to the selected instruction semantics and existing
  prepared-object contracts.

Completion check:

- The selected representative no longer fails with the same
  `unsupported_instruction_fragment` diagnostic.
- Nearby same-fragment cases examined by the executor either advance together
  or are explicitly classified as separate owners.
- Existing backend tests for adjacent instruction lowering remain green.

## Step 3: Prove Representatives And Bucket Movement

Goal: prove the lowering advanced 395 without hiding runtime or ownership
failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for the
  repaired representative and same-fragment additions.
- If a case object-compiles and links, inspect qemu comparison rather than
  stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same instruction-fragment family, a
  distinct target-emission family, or a producer-fact gap that needs a separate
  source idea.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering, or
  filename-specific fixes are used as acceptance evidence.
- The supervisor has enough evidence to continue with another 395 packet,
  request route review, or ask the plan owner for close/deactivation handling.
