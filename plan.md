# Return-Chain Owner/Schema Decision Runbook

Status: Active
Source Idea: ideas/open/176_return_chain_owner_schema_decision.md

## Purpose

Decide where return-chain terminal and next-operand identity should live before
any API contraction or BIR route work happens.

Goal: produce a documented owner/schema decision and bounded follow-up ideas for
the chosen implementation route.

Core Rule: treat return-chain as its own decision; do not fold it into Route 1
producer identity or Route 7 comparison/condition provenance.

## Read First

- `ideas/open/176_return_chain_owner_schema_decision.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`

## Current Targets

- `PreparedReturnChainLookups`
- `prepared_return_chain_value_key(...)`
- `make_prepared_return_chain_lookups(...)`
- `find_prepared_return_chain_terminal_value(...)`
- `find_prepared_return_chain_next_operand_value(...)`
- AArch64 ALU return-chain consumers that recover terminal and next operand
  homes for return-chain emission.

## Non-Goals

- Do not hide or privatize prepared return-chain helpers in this plan.
- Do not add a BIR route before the owner/schema decision is explicit.
- Do not move homes, registers, ALU emission policy, ABI return registers,
  scratch selection, final instruction records, or final instruction order into
  BIR.
- Do not weaken tests, downgrade supported paths, or claim progress through
  expectation rewrites.
- Do not use named-case shortcuts or testcase-shaped matching as a substitute
  for a semantic owner decision.

## Working Model

The current prepared return-chain lookup builds per-position answers for scalar
binary chains inside return blocks. A chain value can map to:

- the terminal return value consumed by the BIR return terminator
- the next operand value used by the immediate next binary in the chain, when
  present and named

AArch64 ALU lowering uses those answers with value homes and return ABI register
selection to retarget intermediate return-chain results and avoid destructive
operand overlap. The decision must separate target-neutral semantic identity
from target-local register/home/emission policy.

## Execution Rules

- Keep analysis facts in a decision artifact or follow-up idea, not in source
  idea churn.
- If the chosen owner is BIR, define accepted cases, rejected cases, stable key
  fields, and exact boundaries against target policy.
- If the chosen owner is target-local AArch64 or deliberately public prepared
  API, explain why future x86/RISC-V lowering should not consume a BIR route for
  this relationship.
- Any code-changing follow-up must first add oracle tests for terminal and
  next-operand answers while the prepared helpers remain public.
- Escalate to broader backend validation in any follow-up that changes public
  headers, `PreparedFunctionLookups`, AArch64 context projection, or route
  facade/schema boundaries.

## Ordered Steps

### Step 1: Inventory Return-Chain Semantics

Goal: record what the existing prepared return-chain data means and how AArch64
uses it.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inspect the producer logic in `make_prepared_return_chain_lookups(...)`.
- List accepted producer shapes: block/terminator requirements, scalar binary
  opcode set, named result requirements, same-block chain traversal, terminal
  return matching, and first-next-operand capture.
- List fail-closed and ambiguous cases: non-return terminator, unnamed return or
  result values, unsupported opcodes, broken chain, duplicate conflicting
  answers, missing value homes, and missing return ABI registers.
- Inventory each AArch64 consumer and separate semantic lookup use from
  target-local home/register/scratch/emission decisions.

Completion check:

- A durable analysis note identifies the current terminal-value and
  next-operand contracts, consumer expectations, and target-policy boundaries.

### Step 2: Decide Owner And Schema Boundary

Goal: choose BIR-owned, target-local AArch64-owned, or explicitly public
prepared-owned return-chain identity.

Actions:

- Evaluate whether terminal and next-operand identity is target-neutral enough
  to become a BIR route.
- If BIR-owned, define the proposed route records, lookup keys, accepted cases,
  rejected cases, stability rules, and relationship to existing BIR block,
  instruction, value, and terminator identity.
- If target-local or prepared-owned, document the reason BIR should not own this
  relationship and what future non-AArch64 targets should do instead.
- Check the decision against Phase B/C reject signals and the source idea's
  out-of-scope boundaries.

Completion check:

- The decision artifact states one owner choice and explains why the other
  choices were rejected without importing target policy into BIR.

### Step 3: Define Proof And Follow-Up Work

Goal: turn the decision into bounded follow-up implementation ideas without
performing the implementation inside this analysis plan.

Actions:

- Define oracle tests needed for terminal and next-operand answers, including
  positive chains and nearby negative/ambiguous cases.
- For a BIR-owned decision, create follow-up ideas for schema/index addition,
  oracle equivalence, consumer migration, and prepared API contraction.
- For a target-local or prepared-owned decision, create follow-up ideas for
  documenting the public/target-local owner and protecting it with focused
  tests.
- Include concrete reviewer reject signals in every new follow-up idea.

Completion check:

- Follow-up ideas under `ideas/open/` are bounded, testable, and sequenced so
  no implementation slice can hide prepared helpers before replacement proof.

### Step 4: Close The Analysis Loop

Goal: make the active analysis plan ready for plan-owner closure review.

Actions:

- Verify `plan.md`, the decision artifact, and any new follow-up ideas agree on
  the selected owner route.
- Confirm no implementation files were changed except as explicitly justified by
  analysis tooling or generated documentation needs.
- Confirm the runbook did not expand into API contraction or schema
  implementation work.

Completion check:

- `todo.md` records the completed analysis result and proof status, and the
  source idea can be judged for closure without treating a future follow-up as
  already complete.
