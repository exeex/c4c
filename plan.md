# Direct-Global Return And Select-Chain Authority Plan

Status: Active
Source Idea: ideas/open/440_direct_global_return_select_chain_authority.md

## Purpose

Define and consume explicit prepared authority for direct-global return values
and direct-global select chains before RV64 lowering materializes them.

## Goal

Classify direct-global return/select-chain residuals, define the prepared
authority needed to publish those facts, and select only bounded producer or
consumer packets that consume explicit prepared authority.

## Core Rule

Do not infer return authority, select-chain roots, global pointer identity, or
publication intent from raw `bir.ret ptr @global`, global symbol names, select
instruction shape, testcase names, function names, or one prepared dump layout.

## Read First

- ideas/open/440_direct_global_return_select_chain_authority.md
- ideas/closed/433_rv64_global_select_pointer_memory_residuals.md
- ideas/closed/439_store_source_global_memory_publication_authority.md
- ideas/closed/446_global_memory_layout_authority_publication.md
- ideas/closed/447_immediate_global_store_source_encoding.md
- ideas/closed/448_array_aggregate_global_layout_authority.md
- build/agent_state/433_step4_residual_disposition/classification.md
- build/agent_state/433_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/447_step4_residual_disposition/classification.md
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Direct-global return and select-chain evidence from the 433 Step 4 residual
  disposition.
- `20041112-1` direct-global facts, including raw `bir.ret ptr @global` and
  direct-global select-chain rows.
- Prepared facts that authorize returning a global pointer and materializing
  direct-global select-chain roots.
- Focused tests that distinguish accepted direct-global authority from missing
  or incoherent authority.

## Non-Goals

- Selected global object-data support completed by idea 433.
- Store-source/global-memory publication completed by ideas 439, 446, 447, and
  448.
- General terminator/select publication not tied to direct-global return or
  select-chain authority; that belongs to idea 441.
- Pointer-value memory authority, local/frame-slot publication, or generic RV64
  instruction-fragment lowering.
- RV64 target-side guessing from raw BIR, select shape, symbol spelling, or
  representative testcase shape.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

The global-memory producer prerequisites are now separated or closed, but
direct-global returns and select chains still need explicit prepared authority.
This plan owns the direct-global authority question only: either prepared facts
prove a bounded return/select-chain route, or the row remains fail-closed and
any broader terminator work stays separate.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat raw `bir.ret ptr @global` and raw select-chain shape as insufficient
  authority.
- Separate producer gaps from coherent RV64 consumer work.
- Add focused coverage for accepted direct-global facts and fail-closed
  missing/incoherent authority.
- Select at most one narrow code packet after the authority contract is
  explicit.
- If the first owner is broad terminator/select publication, record the route
  instead of widening this plan into idea 441.
- Do not touch `test_baseline.new.log`, `test_before.log`, or
  `test_after.log`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Direct-Global Evidence

Inspect the 433 and 447 residual evidence for direct-global return and
select-chain rows. Record each row's function/block, raw BIR shape, prepared
fact, global symbol, value home or terminator use, current authority state, and
first missing producer or consumer fact. Completion means `todo.md` contains a
bucket table and identifies the first bounded direct-global authority packet or
exact blocker.

### Step 2: Define Direct-Global Authority Contract

Specify the prepared facts required to authorize direct-global return values
and direct-global select-chain roots. Separate accepted records from producer
gaps, incoherent facts, and general terminator/select publication. Completion
means `todo.md` states the contract, rejected adjacent shapes, owned
files/tests, and proof command.

### Step 3: Implement Or Route First Direct-Global Packet

If a coherent producer/prepared or RV64 consumer packet exists, implement the
smallest semantic change with focused coverage. If the first owner is a
distinct producer gap or broader terminator/select publication, record the
split or blocker instead of broadening this source. Completion means proof
passes or canonical lifecycle state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative direct-global return/select-chain rows against the
Step 3 result, classify remaining residuals, and decide whether this source
idea is complete. Completion means close, keep active with an exact remaining
direct-global packet, or route durable follow-up work.
