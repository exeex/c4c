# Stack-Restore Lifetime-Consumer Authority Runbook

Status: Active
Source Idea: ideas/open/798_lir_stack_restore_lifetime_consumer_authority.md
Activated from: separate-blocker switch from 794 after commit `1cbad00d6`.

## Purpose

Publish and verify exactly one native stack-restore lifetime-consumer
authority route so 794 can later resume its one-row handoff process.

## Core Rule

Admit only `LirStackRestoreOp` through checked native current-function facts.
Do not infer row identity, liveness, or lifetime transition from names,
formatted operands, LLVM text, testcase identity, `monostate`, or an
unresolved classification.

## Read First

- `ideas/open/798_lir_stack_restore_lifetime_consumer_authority.md`
- `ideas/open/794_lir_next_local_vla_authority_handoff.md`
- `docs/lir_to_new_bir_remaining_coverage/794_local_vla_candidate_evidence_boundary.md`
- `docs/lir_local_operation_authority/handoff_to_734.md`

## Non-Goals

- Raw-BIR/importer/734 receipt, lowering, dynamic-VLA count work, VLA GEP,
  other local rows, or a generic lifetime-model redesign;
- rendered-text or testcase-derived authority, weaker verifier contracts, or
  multiple operation admissions.

## Ordered Steps

### Step 1 - Define the selected native stack-restore authority contract

Goal: establish the exact producer/schema facts that select one
`LirStackRestoreOp` and encode its lifetime-consumer transition alongside its
existing saved-pointer binding.

Actions:

- Identify the minimal operation-specific admission and structured transition
  facts needed for this one restore row.
- Keep all dynamic-VLA, VLA-GEP, other local, and receiver work fail closed.

Completion check: the selected schema/producer contract is explicit, native,
and sufficient for verifier implementation without presentation-derived facts.

### Step 2 - Verify the selected stack-restore authority

Goal: enforce the selected admission and reject malformed or foreign
saved-pointer, owner, type, liveness, and transition facts.

Completion check: focused same-feature positive/negative proof accepts the
valid selected restore and rejects each invalid authority class.

### Step 3 - Publish the exact return handoff to 794

Goal: record the exact native fields, rejected forms, and accepted focused
proof needed for 794 to resume at its Step 2.

Completion check: the handoff authorizes only 794's selected-authority
publication process and explicitly does not authorize 734 receipt or another
local/VLA row.
