# 813 LIR String Semantic Authority Completion Runbook

Status: Active
Source Idea: ideas/open/813_lir_string_semantic_authority_completion_umbrella.md
Activated from: no-active-plan routing after closure commit `bae8bf4ae`

## Purpose

Resume the 813 umbrella route that converts accepted 812 evidence into exact
owners, dependency order, and terminal handoff material for 734 and 797.

## Goal

Finish 813's evidence-to-owner mapping without implementation changes, duplicate
successors, or premature 797 convergence.

## Core Rule

813 is routing work only. It may edit its handoff documents and, if required,
create separately scoped open source ideas. It must not edit implementation
code, tests, runtime behavior, active unrelated sources, or treat classification
as semantic capability.

## Read First

- `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/closed/848_lir_global_policy_identity_evidence.md`
- `ideas/closed/849_lir_intrinsic_binding_evidence.md`
- `ideas/closed/850_lir_cfg_phi_raw_bindings_evidence.md`
- `ideas/closed/847_lir_universal_model_string_escape_hatch_deletion.md`

## Current Targets

- Step 1 is accepted historical work in commit `1a58beed1`.
- Evidence-route sources 848, 849, and 850 are now closed and must be reconciled
  against the three stable 812/813 keys.
- The 847 deletion handoff is available evidence for final valid-LIR
  disposition, but it does not make 797 eligible while 813 residual routing
  remains unfinished.
- Preserve existing owner scopes and return chains. Do not duplicate work
  already fully owned by an open or closed source.

## Non-Goals

- Do not implement LIR, BIR, frontend, backend, test, or runtime changes.
- Do not promote or activate draft 837.
- Do not activate 734 or 797 from this runbook.
- Do not create broad catch-all successors for "remove LIR strings" or mixed
  producer/receiver/proof ownership.
- Do not infer semantic authority from rendered text, names, operands,
  diagnostics, printer output, or testcase spelling.

## Working Model

The accepted 812 input contains exactly three insufficient-evidence keys:

1. `global.policy-identity-evidence`
2. `instruction.intrinsic-binding-evidence`
3. `cfg.phi-raw-bindings-evidence`

Closed 848, 849, and 850 answer those evidence questions. Step 2 must now map
each key exactly once to a final disposition: existing owner, completed
evidence/no direct receiver, or a newly created first-owner successor if the
closure records prove one is still required. Step 3 publishes the dependency
queue and handoff to 734/797.

## Execution Rules

- Use the closed source closure notes as evidence; do not rerun broad
  validation.
- Prefer handoff-document edits over source-idea edits unless durable source
  intent changes or a new open successor is required.
- If a new successor is required, create one source under `ideas/open/` with
  explicit goal, scope, non-goals, acceptance criteria, and
  `## Reviewer Reject Signals`.
- Keep 797 downstream until every valid current-LIR row has an explicit typed,
  checked-mirror, intentional-text, completed-evidence, or successor-owned
  disposition.
- Use documentation proof only for documentation-only steps, such as
  `git diff --check` and exact file inventory checks.

## Step 1 - Validate 812 Input And Refresh Lifecycle State

Goal: Confirm the accepted 812 evidence revision and current lifecycle state.

Status: Complete.

Accepted evidence:
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- commit `1a58beed1`
- proof: `git diff --check > test_after.log`

Completion check: already satisfied. Do not repeat unless the current evidence
documents are missing or contradictory.

## Step 2 - Assign First Owners And Bounded Evidence Routes

Goal: Reconcile the three stable keys against closed 848, 849, 850, and 847,
then publish exact owner/disposition maps.

Primary targets:
- `docs/lir_string_semantic_authority_completion/row_to_owner_map.md`
- `docs/lir_string_semantic_authority_completion/existing_owner_dependencies.md`

Actions:
- Read the closure notes for 848, 849, and 850 and map each stable key exactly
  once.
- Record that 848 and 849 are evidence-complete but authorize no direct 734
  receiver handoff; carry their future owner/downstream relations exactly.
- Record that 850 is evidence-complete and authorizes no new direct 734
  receiver handoff because accepted 734 bounded receipts already cover the
  modeled CFG/PHI receiver rows described by that evidence.
- Incorporate 847 as terminal deletion evidence for 797 only, with an explicit
  note that 812/813 residual non-type/string routes and switch selector
  surfaces remain outside that handoff.
- Decide whether any closure note requires a new open first-owner successor. If
  yes, create only the exact successor required by that closure record. If no,
  state the completed-evidence/no-direct-receiver disposition explicitly.
- Preserve existing open-owner scopes for 734, 795, 796, 797, 821, and 822.

Completion check:
- Every stable key appears exactly once in `row_to_owner_map.md`.
- Every reused, deferred, downstream, or successor owner relation appears in
  `existing_owner_dependencies.md`.
- No row is silently omitted, duplicated, or assigned to 797 as a repair owner.
- Documentation proof passes with a narrow file inventory check and
  `git diff --check`.

## Step 3 - Publish Queue, Architecture Alignment, And Terminal Handoff

Goal: Finish 813's handoff package after Step 2 owner/disposition mapping is
accepted.

Primary targets:
- `docs/lir_string_semantic_authority_completion/successor_queue.md`
- `docs/lir_string_semantic_authority_completion/architecture_alignment.md`
- `docs/lir_string_semantic_authority_completion/handoff_to_734_and_797.md`
- `docs/lir_string_semantic_authority_completion/closure_trace.md`

Actions:
- Publish the dependency order from Step 2, including any generated successors
  or completed-evidence dispositions.
- Record draft 837 only as architecture input while it remains parked.
- State exactly what, if anything, can return to 734, and what remains terminal
  input to 797.
- Include 847 as a deletion-route handoff to 797 without claiming final 797
  convergence.
- Prepare a closure trace that lists accepted 812 revision, lifecycle refresh,
  every reused owner, every completed evidence route, any generated successor,
  excluded rows, and final downstream ownership.

Completion check:
- All required 813 handoff documents exist and agree with each other.
- The closure trace leaves no unresolved 812 key without one owner,
  completed-evidence disposition, or evidence-backed successor.
- Narrow documentation proof passes.

## Step 4 - Request Lifecycle Close Decision

Goal: Hand 813 back to plan-owner for closure judgment after the handoff package
is complete and accepted.

Actions:
- Supervisor verifies Step 2 and Step 3 documentation proof.
- Plan-owner decides whether 813 closure is accepted or whether a runbook
  repair or separately scoped blocker is required.

Completion check:
- If accepted, close 813 with a closure note naming the accepted 812 evidence
  revision, lifecycle refresh, closed evidence routes 848/849/850, 847 handoff
  relation, dependency order, and terminal 734/797 disposition.
