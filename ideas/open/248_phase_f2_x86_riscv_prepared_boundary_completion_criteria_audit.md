# 248 Phase F2 x86/riscv prepared boundary completion criteria audit

## Goal

Define the concrete completion criteria for saying the BIR/prepared boundary is
clean enough to support x86/riscv portability work, then produce follow-up
ideas for the exact blocker families that still fail those criteria.

This is analysis-only. It must not implement adapters, delete prepared fields,
open draft 155, or claim prepared aggregate retirement readiness. Its job is to
turn the Phase F1 blocker map into precise "done enough to connect x86/riscv"
criteria and smaller follow-up ideas.

## Why This Exists

Ideas 243 through 247 already completed one portability and demotion-readiness
pass:

- `ideas/closed/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md`
- `ideas/closed/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`
- `ideas/closed/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`
- `ideas/closed/246_phase_f1_prepared_publication_status_compatibility_retention.md`
- `ideas/closed/247_phase_f1_final_prepared_field_group_demotion_gate.md`

That pass proved selected x86/riscv route-native diagnostics, adapters,
compatibility-retention rows, and wrapper-output baselines. It also concluded
that no `PreparedBirModule` or `PreparedFunctionLookups` field group is ready
for final deletion, demotion, or privatization.

The remaining problem is not simply "more cleanup." The remaining problem is
that the BIR/prepared ownership boundary is not yet specified tightly enough to
decide when x86 and riscv can share the same semantic facts without prepared
still acting as a second semantic IR.

## Completion Target

The boundary is clean enough to connect x86/riscv only when each relevant
field group or fact family has one explicit classification:

- BIR-owned semantic fact;
- target-owned policy product;
- private prepared pass context;
- retained public compatibility adapter;
- blocked public prepared authority;
- deletion or demotion candidate.

For every fact family that x86 and riscv both need, the audit must define what
"done enough" means:

- the positive semantic fact is route/BIR-owned;
- x86 and riscv consume the same fact family, or the target-specific gap is
  explicitly named;
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  fallback, and policy-sensitive cases fail closed;
- wrapper output, route-debug output, helper/oracle statuses, expected strings,
  and baselines remain stable;
- ABI, layout, register, stack, storage, addressing, carrier/runtime helper,
  formatting, emission, and wrapper policy remain outside BIR ownership;
- prepared public APIs are either retained as named compatibility adapters or
  demoted behind private pass context with no public consumer left.

## In Scope

- Re-reading closure notes from 243 through 247.
- Inspecting the current ownership boundary for:
  - `call_plans`;
  - `memory_accesses`;
  - `edge_publications`;
  - `edge_publication_source_producers`;
  - `module`;
  - `names`;
  - `control_flow`;
  - `store_source_publications`;
  - prepared helper/oracle/status/fallback/route-debug/wrapper-output
    compatibility surfaces.
- Defining separate completion criteria for:
  - route/BIR semantic ownership;
  - x86 consumption;
  - riscv consumption;
  - public prepared compatibility retention;
  - target policy retention;
  - private pass-context demotion;
  - field-group deletion/demotion readiness.
- Producing follow-up ideas that are narrow enough for an agent to execute
  without broad route drift.

## Out Of Scope

- Direct implementation of any adapter or demotion.
- Broad deletion of `PreparedBirModule` or `PreparedFunctionLookups`.
- Opening draft 155 as a broad retirement plan.
- Claiming field-group readiness from one-reader, x86-only, riscv-only,
  diagnostic-only, compatibility-only, or baseline-only proof.
- Moving target policy into BIR.
- Renaming, weakening, hiding, or deleting prepared compatibility surfaces as a
  substitute for ownership transfer.

## Required Analysis

### 1. Boundary Classification Matrix

Build a matrix for the remaining blocker groups named by idea 247. Each row
must classify the group as BIR semantic, target policy, private pass context,
retained compatibility, blocked public authority, deletion candidate, or mixed.

Mixed groups must be split into smaller candidate rows. Do not leave a row
with only "mixed" as the conclusion.

### 2. x86/riscv Portability Readiness Criteria

For each candidate fact family, state the minimum evidence needed before it can
be called portable across x86/riscv:

- which BIR or route fact is the semantic authority;
- which x86 consumer must read it;
- which riscv consumer must read it;
- which prepared answer must remain as agreement/fallback;
- which diagnostics and strings must remain stable;
- which target-policy surfaces are adjacent and must not move.

### 3. Prepared Public API Exit Criteria

For each public prepared API or lookup group, state whether it can eventually:

- remain public as compatibility;
- become private pass context;
- split into target-policy product plus BIR-owned semantic index;
- be deleted after all public consumers move;
- stay blocked.

The audit must name the observable condition that makes each transition safe.

### 4. Follow-Up Idea Generation

Create follow-up ideas only when the criteria are concrete enough for bounded
execution. Each generated idea must say whether it is:

- analysis-only;
- one-adapter implementation;
- compatibility-retention proof;
- x86/riscv parity proof;
- private-pass-context demotion;
- deletion/demotion gate.

Prefer one fact family or one field-group sub-slice per idea. Do not create a
large "retire prepared" implementation idea.

## Expected Output

The closure note must include:

- a boundary classification matrix for the blocker groups from idea 247;
- a definition of "clean enough to connect x86/riscv" in operational terms;
- a list of x86/riscv fact families that are portable, partially portable, or
  blocked;
- a list of prepared public APIs that remain compatibility authority;
- a list of target-policy surfaces that must not move to BIR;
- a list of candidate private pass-context demotions, if any;
- follow-up idea files for every next executable packet;
- a disposition for draft 155: keep blocked, rewrite, supersede, or retire
  obsolete.

If no implementation packet is safe, close with a blocker map and analysis-only
follow-up ideas instead of forcing demotion work.

## Reviewer Reject Signals

- Treating this as another vague audit without concrete completion criteria.
- Claiming "BIR owns semantics" while prepared public APIs still provide the
  observable semantic answer.
- Treating x86 evidence as riscv readiness, or riscv evidence as x86 readiness.
- Treating retained compatibility strings as proof that ownership transferred.
- Creating broad prepared retirement work instead of bounded fact-family or
  field-group follow-ups.
- Hiding public prepared authority behind private wrapper names.
- Moving ABI, register, stack, storage, addressing, formatting, emission, or
  wrapper policy into BIR.
