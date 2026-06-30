# RV64 Global Select Pointer Memory Residuals Plan

Status: Active
Source Idea: ideas/open/433_rv64_global_select_pointer_memory_residuals.md

## Purpose

Resolve residual RV64 object-route failures left after bounded pointer-cast
materialization by separating coherent target-lowering work from
producer/prepared gaps.

## Goal

Classify global object-data, global pointer-memory publication,
direct-global select/return, terminator/select-publication, and pointer-value
memory residuals into owner buckets with representative evidence, then execute
only packets that consume explicit prepared facts.

## Core Rule

RV64 lowering must not infer global object identity, symbol addressability,
relocation base, memory layout, select-chain roots, return pointer authority, or
pointer-value memory ownership from filenames, raw BIR shape, or integer values.

## Read First

- ideas/open/433_rv64_global_select_pointer_memory_residuals.md
- build/agent_state/429_step4_close_readiness/classification.md
- build/agent_state/429_step4_close_readiness/probe_summary.tsv
- build/agent_state/429_step4_close_readiness/evidence_snippets.txt
- build/agent_state/429_step4_close_readiness/*.prepared.out
- build/agent_state/429_step4_close_readiness/*.object.err
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp
- src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp
- src/backend/mir/riscv/codegen/prepared_frame_emit.cpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- Residual rows from `build/agent_state/429_step4_close_readiness/`.
- Global object-data failures.
- Global pointer load/store publication.
- Direct-global select chains.
- Direct `bir.ret ptr @global`.
- Terminator/select publication.
- Pointer-value memory residuals.
- Producer/prepared gaps among those classes.

## Non-Goals

- Reopening bounded `inttoptr`/`ptrtoint` movement completed by idea 429.
- Guessing object identity, addressability, relocation base, memory layout,
  select-chain roots, or return pointer authority.
- Aggregate ABI, F128, scalar FP, div/rem, or unrelated call-publication work.
- Expectation rewrites, allowlists, unsupported downgrades, runtime-only
  rewrites, or pass/fail accounting changes.

## Working Model

Some residuals may be coherent RV64 target work if prepared facts explicitly
describe object identity, addressability, memory, select, or return authority.
Other residuals are missing or incoherent prepared facts that need separate
producer ownership. Step 1 is classification; no implementation should begin
until the owner bucket is clear.

## Execution Rules

- Start by reclassifying evidence; Step 1 is lifecycle/triage only.
- If target work exists, pick one narrow semantic packet that consumes explicit
  prepared facts.
- If a producer gap exists, record follow-up idea details instead of bypassing
  the missing fact in RV64 lowering.
- Keep the target/producer boundary explicit in `todo.md`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if a later packet reaches implementation:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Reclassify Residual Evidence By First Owner

Inspect `build/agent_state/429_step4_close_readiness/` and group rows into
first-owner buckets: global object-data, global memory publication,
direct-global select/return, terminator/select publication, pointer-value
memory, and producer/prepared gaps. Completion means `todo.md` records a bucket
table, representative rows/evidence, and the first executable packet or the
exact reason no target packet is coherent.

### Step 2: Select First Coherent Packet Or Producer Split

Choose one narrow packet with explicit prepared authority, or produce
source-idea-ready producer split notes if no target packet is coherent.
Completion means `todo.md` states the selected packet, owned files, focused
proof, and guardrails, or states the exact producer gap to route.

### Step 3: Execute Or Route Residual Packet

For a coherent target packet, implement bounded semantic lowering with focused
tests and backend proof. For a producer/prepared gap, create or switch to a
separate lifecycle idea. Completion means proof passes for the implementation
slice or canonical lifecycle state records the split.

### Step 4: Residual Disposition And Close Readiness

Classify remaining residuals, create durable follow-up ideas if needed, and
decide whether the source idea is close-ready. Completion means the idea is
closed/deactivated, or remains active with an exact remaining in-scope
target-lowering packet.
