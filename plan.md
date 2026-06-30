# Store-Source Global-Memory Publication Authority Plan

Status: Active
Source Idea: ideas/open/439_store_source_global_memory_publication_authority.md

## Purpose

Repair or classify store-source and global-memory publication authority so RV64
global load/store lowering only consumes explicit prepared facts.

## Goal

Classify residual global memory publication rows into coherent target work
versus producer/prepared gaps, define the required store-source/global-memory
facts, and select only implementation packets with explicit authority.

## Core Rule

Do not infer store-source identity, global object identity, offsets, layout,
addressability, or value homes in RV64 from raw BIR, testcase names, object
labels, symbol spelling, or dump shape.

## Read First

- ideas/open/439_store_source_global_memory_publication_authority.md
- build/agent_state/433_step4_residual_disposition/classification.md
- build/agent_state/433_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/433_step4_residual_disposition/930930-1.prepared.out
- build/agent_state/433_step4_residual_disposition/20000622-1.prepared.out
- build/agent_state/433_step4_residual_disposition/20041112-1.prepared.out
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp
- src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Global memory publication residuals in `930930-1`, `20000622-1`, and
  `20041112-1`.
- Store-source records with `source_producer=unknown`.
- Prepared global symbol accesses, offsets, widths, and value homes that may
  be coherent target inputs.
- Producer/prepared gaps where store source, global layout, or value home is
  not explicit.

## Non-Goals

- Reopening selected global object-data support completed by idea 433.
- Pointer-value memory layout/provenance authority, including the parked 442
  external-linkage formal route.
- Direct-global return/select-chain authority.
- Terminator/select publication.
- RV64 inference from raw BIR or representative filenames.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, or pass/fail accounting changes.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

Some residual global memory operations may be target-consumable if prepared
facts explicitly identify the global symbol, offset, width, addressability, and
store source/value home. Others are producer gaps, especially
`source_producer=unknown`, and must remain fail-closed or split to separate
producer initiatives.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat unknown store source or missing global publication authority as
  fail-closed.
- Select at most one narrow packet after classification.
- Add focused tests for semantic producer/prepared facts, not named testcase
  shortcuts.
- If a target packet is selected, it must consume explicit prepared facts.
- Do not touch `test_baseline.new.log`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Store-Source And Global-Memory Evidence

Inspect the residual rows from the 433 Step 4 evidence. For each global memory
publication or store-source residual, record the symbol, offset, width, stored
or loaded value, current producer, value home, and first missing fact.
Completion means `todo.md` contains a bucket table and the first executable
packet or exact blocker.

### Step 2: Define Publication Authority Contract

Specify the prepared facts required for store-source and global-memory
publication authority. Separate accepted target-consumable records from
producer gaps such as `source_producer=unknown`. Completion means `todo.md`
states the contract, rejected adjacent shapes, owned files/tests, and proof
command.

### Step 3: Implement Or Route First Publication Packet

If a coherent producer/prepared or target packet exists, implement the smallest
semantic change with focused coverage. If the first owner is a distinct
producer gap, create or request a separate lifecycle idea instead of widening
this source. Completion means proof passes or canonical lifecycle state records
the split.

### Step 4: Residual Disposition And Close Readiness

Re-check the representative rows, classify remaining residuals, and decide
whether this source idea is complete. Completion means close, keep active with
an exact remaining publication packet, or route durable follow-up work.
