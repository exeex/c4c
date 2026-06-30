# Direct-Global Return And Select-Chain Authority Plan

Status: Active
Source Idea: ideas/open/440_direct_global_return_select_chain_authority.md

## Purpose

Define and consume explicit prepared authority for direct-global return values
and direct-global select chains before RV64 lowering materializes them.

## Goal

Finish the direct-global return path by consuming the explicit prepared return
authority already added in Step 3, while keeping direct-global select-chain and
generic terminator/select publication as separate work.

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

- `20041112-1 foo.block_1` direct-global return through raw
  `bir.ret ptr @global`.
- The prepared `@global` home/value fact with `value_id=4`, register-backed
  storage, and matching `BeforeReturn` `FunctionReturnAbi` move to `a0`.
- `plan_prepared_direct_global_return_authority` and
  `prepared_direct_global_return_authority_available` as the only accepted
  authority surface for this consumer packet.
- Focused RV64 consumer tests for accepted authority and fail-closed raw-only,
  missing-authority, mismatched-home, non-global, unsupported-home, and non-GPR
  return shapes.

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

The global-memory producer prerequisites are separated or closed, and Step 3
added the prepared direct-global return authority predicate. The remaining
in-scope work is a narrow RV64 object consumer for that explicit authority. The
consumer must not generalize to raw global returns, direct-global select-chain
rows, or generic terminator/select publication.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat raw `bir.ret ptr @global` and raw select-chain shape as insufficient
  authority.
- Separate producer gaps from coherent RV64 consumer work.
- Add focused coverage for accepted direct-global facts and fail-closed
  missing/incoherent authority.
- The next code packet must consume only
  `plan_prepared_direct_global_return_authority`/prepared homes and matching
  `BeforeReturn` ABI move facts.
- Fail closed for raw-only, missing-authority, mismatched-home, non-global,
  unsupported-home, non-pointer, and non-GPR ABI return shapes.
- Keep `direct_global_select_chain=yes` rows and generic terminator/select
  publication separate unless a later lifecycle review explicitly selects
  them.
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

### Step 5: Consume Direct-Global Return Authority

Implement the narrow RV64 object-route consumer for `20041112-1 foo.block_1`
by materializing `bir.ret ptr @global` only when explicit prepared
direct-global return authority is available. The consumer must use
`plan_prepared_direct_global_return_authority`, the prepared global
home/storage, and the matching `BeforeReturn` `FunctionReturnAbi` GPR move as
authority. It must not infer from raw BIR, symbol spelling, function names, or
the generic terminator diagnostic. Completion means the accepted direct-global
return route is covered by focused tests, fail-closed tests cover raw-only,
missing-authority, mismatched-home, non-global, unsupported-home, non-pointer,
and non-GPR return shapes, the delegated proof passes, and `todo.md` records
whether any residual direct-global return work remains.

### Step 6: Final Residual Disposition And Close Readiness

Re-probe the representative after Step 5, separate direct-global return
residuals from direct-global select-chain and generic terminator/select
publication, and decide whether idea 440 is complete. Completion means close
440, keep it active with an exact remaining direct-global return/select-chain
packet, or route durable follow-up work.
