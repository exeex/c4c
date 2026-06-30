# Branch Stack-Load Authority Metadata Plan

Status: Active
Source Idea: ideas/open/469_branch_stack_load_authority_metadata.md
Activated From: ideas/closed/451_stack_home_branch_operand_materialization.md

## Purpose

Define and publish producer/prepared authority for loading stack-home branch
operands or conditions at a branch site.

## Goal

Make stack-home branch materialization target-consumable only through explicit
prepared branch-stack-load authority records, with unavailable statuses for
missing or incoherent facts.

## Core Rule

Do not treat stack slot ids, offsets, homes, or object facts as sufficient
target authority. Branch stack loads require explicit load policy, freshness,
clobber-safety, scratch/order, branch identity, operand role, and pointer
status metadata.

## Read First

- ideas/open/469_branch_stack_load_authority_metadata.md
- ideas/closed/451_stack_home_branch_operand_materialization.md
- build/agent_state/451_step2_stack_home_branch_contract/contract.md
- build/agent_state/451_step3_stack_home_branch_materialization_route/route.md
- build/agent_state/451_step4_residual_disposition/disposition.md
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- Primary representative candidate: `930930-1`, `f.block_1`,
  `%t2 = ult ptr %t1, %p.reg2`.
- Needed metadata: branch site, prepared branch-condition identity, operand
  role, stack slot/object facts, explicit load policy, freshness,
  clobber-safety, scratch/order constraints, and pointer-provenance status.
- Adjacent rows to preserve as separate:
  - `%t7/%t8` pointer-value/provenance dependency;
  - `%t22/%t23` select-result/block-entry stack-destination dependency;
  - GPR-compatible branches already closed by earlier routes.

## Non-Goals

- RV64 branch-load emission before available metadata exists.
- Weakening GPR-compatible branch predicates.
- Inferring branch loads, freshness, operands, or conditions from raw BIR,
  stack-slot spelling, block order, filenames, function names, or one prepared
  dump.
- Pointer-value/provenance repair for `%t7` or external formal provenance.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The RV64 consumer is blocked because the prepared layer does not yet state
whether a branch-site stack load is valid. This plan owns the producer/prepared
authority surface, not target emission. Any accepted record must prove the
slot contains the current branch operand/condition value and can be loaded
without clobbering or stale data.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Publish metadata records before selecting RV64 consumers.
- Preserve fail-closed behavior for missing policy, missing freshness,
  clobbered slot, pointer-provenance unknowns, select-result stack
  destinations, wrong branch/role/slot/object, and raw-shape-only fixtures.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a metadata packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Branch Stack-Load Metadata Inputs

Inspect representative stack-home branch rows and existing prepared facts.
Record branch identity, operand role, value home, stack slot/object match,
current load/freshness/clobber evidence, pointer status, and first missing
metadata field. Completion means `todo.md` contains an audit table and
identifies the first metadata packet or exact blocker.

### Step 2: Define Branch Stack-Load Authority Contract

Specify available/unavailable record fields and fail-closed statuses for
branch-stack-load authority. Completion means `todo.md` records positive and
negative cases and names target files/tests if implementation is justified.

### Step 3: Implement Or Route Metadata Packet

If Step 2 finds a bounded producer/prepared packet, implement only that
metadata surface with focused coverage. If no implementation is justified,
record the precise blocker and route it. Completion means proof passes or
lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe representative rows and classify any remaining first owner.
Completion means this source idea closes, remains active with one exact
in-scope metadata packet, or routes an RV64 consumer / other durable follow-up.
