# Terminator Select Publication Authority Plan

Status: Active
Source Idea: ideas/open/441_terminator_select_publication_authority.md

## Purpose

Establish explicit publication authority for terminator/select residuals so
RV64 control-flow lowering consumes prepared facts instead of reconstructing
operands from raw BIR shape.

## Goal

Classify terminator/select publication residuals, define the prepared facts
needed for branch/return/select operands, and select only bounded producer or
consumer packets that consume explicit publication facts.

## Core Rule

Do not infer terminator operands, select results, branch conditions, compare
inputs, or publication homes from raw BIR shape, block order, filenames,
function names, testcase names, or one prepared dump layout.

## Read First

- ideas/open/441_terminator_select_publication_authority.md
- ideas/closed/433_rv64_global_select_pointer_memory_residuals.md
- ideas/closed/440_direct_global_return_select_chain_authority.md
- build/agent_state/433_step4_residual_disposition/classification.md
- build/agent_state/433_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/440_step6_final_residual_disposition/classification.md
- build/agent_state/440_step6_final_residual_disposition/20041112-1.prepared.out
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20041112-1 bar.entry` fused pointer compare branch:
  `%t6 = bir.ne ptr %t2, %t5` feeding
  `bir.cond_br i32 %t6, block_4, block_5`.
- Prepared `branch_condition entry kind=fused_compare` evidence from the 440
  Step 6 final residual disposition.
- Terminator/select residuals from the 433 Step 4 evidence, including
  `20010329-1`, `20041112-1`, `20000622-1`, and `930930-1`.
- Prepared facts needed to publish select results, fused compares, branch
  conditions, return operands, or value homes into terminator lowering.

## Non-Goals

- Direct-global return authority completed by idea 440.
- Selected global object-data support completed by idea 433.
- Store-source/global-memory publication and layout authority completed by
  prior global publication ideas.
- Pointer-value memory authority and provenance publication.
- Generic RV64 terminator lowering from raw BIR without explicit prepared
  publication facts.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.
- Touching `review/440_step5_direct_global_return_consumer_review.md`.

## Working Model

The closed 440 route consumed explicit direct-global return authority, but the
representative still fails in `bar.entry` through a fused pointer compare that
feeds a conditional branch. This plan owns the prepared publication boundary
for terminator/select operands: either prepared facts explicitly authorize a
bounded branch/select/return consumer, or the residual remains fail-closed and
is routed to a more specific producer idea.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat raw `bir.cond_br`, raw `bir.ne`, select instruction shape, and block
  order as insufficient authority.
- Separate prepared publication producer gaps from coherent RV64 consumer
  work.
- Keep direct-global return/select-chain authority out unless Step 1 isolates a
  terminator/select publication packet that explicitly depends on it.
- Add focused tests for accepted publication facts and fail-closed
  missing/incoherent boundaries before claiming RV64 progress.
- Select at most one narrow code packet after the publication contract is
  explicit.
- Do not touch `test_baseline.new.log`, `test_before.log`, `test_after.log`,
  or `review/`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Terminator/Select Publication Evidence

Inspect the 433 Step 4 and 440 Step 6 residual evidence for terminator/select
publication rows. Record each row's function/block, raw terminator or select
shape, prepared branch/select/publication fact, source value homes, compare
operands, result type, consumer boundary, current authority state, and first
missing producer or consumer fact. Completion means `todo.md` contains a
bucket table and identifies the first bounded terminator/select publication
packet or exact blocker.

### Step 2: Define Terminator/Select Publication Contract

Specify the prepared facts required to authorize branch conditions, fused
compare conditions, select result publication, return operands, and value homes
for terminator lowering. Separate accepted records from producer gaps,
incoherent facts, and unrelated direct-global or memory-publication residuals.
Completion means `todo.md` states the contract, rejected adjacent shapes, owned
files/tests, and proof command.

### Step 3: Implement Or Route First Publication Packet

If a coherent producer/prepared or RV64 consumer packet exists, implement the
smallest semantic change with focused coverage. If the first owner is a
distinct producer gap or a narrower authority family, record the split or
blocker instead of broadening this source. Completion means proof passes or
canonical lifecycle state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative terminator/select rows against the Step 3 result,
classify remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining
terminator/select-publication packet, or route durable follow-up work.
