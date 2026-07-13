# Inline-Assembly Preparation And Regalloc Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Supersedes: the paused idea-730 globals packet; its WIP remains in `stash@{0}`

## Purpose

Align inline asm with the ordered replacement backend: target-independent
transport through Canonical BIR, target-aware constraint resolution in
preparation, structured MIR/regalloc consumption, and instruction parsing only
at late assembly.

## Goal

Preserve inline asm faithfully while giving regalloc verified target-specific
requirements for `r`, `=r`, ties/clobbers, and RV64 vector register groups.

## Core Rule

Canonical BIR carries source semantics, not target interpretation. Only
`preparation/inline_asm` may normalize constraints and clobbers; only the late
assembler may parse instruction/template syntax.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/backend/bir/README.md`
- `src/backend/bir/pipeline/README.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`
- `src/backend/bir/core/README.md`
- `src/backend/bir/verify/README.md`
- `src/backend/bir/preparation/README.md`
- `src/backend/bir/preparation/inline_asm/README.md`
- `docs/inline_asm_transport/`

## Current Targets

- ordered-architecture and checkpoint acceptance
- structured opaque inline-asm transport into Canonical BIR
- revision-bound target-aware inline-asm preparation
- verified `PreparedBir` publication and a bounded handoff to new MIR
- structured, string-free regalloc requirements
- late assembler substitution and first parse

## Non-Goals

- Do not implement while architecture/checkpoint acceptance is incomplete.
- Do not put RV64 normalization, physical interpretation, target-contract
  provenance, or allocator requirements in canonical BIR.
- Do not let preparation mutate `CanonicalBir` or communicate through unnamed
  side tables.
- Do not parse instructions/templates before late assembly.
- Do not revive legacy BIR, prealloc, MIR, or `c4c-as`.
- Do not apply `stash@{0}` or resume idea 730 in this runbook.

## Working Model

1. Parser/HIR/LIR preserve the opaque payload, structured operands, and
   original constraint/clobber syntax.
2. LIR-to-BIR constructs target-independent Raw BIR; canonical passes publish
   verified `CanonicalBir` without normalizing target syntax.
3. `preparation/inline_asm` reads immutable `CanonicalBir` plus target context
   and produces a typed immutable plan bound to the exact canonical revision
   and target-context identity/version.
4. The ordered planners and Prepared-input verification publish one verified
   `PreparedBir` containing that typed plan component.
5. MIR construction consumes verified `PreparedBir` (or its verified read-only
   view), never raw `CanonicalBir` plus a separately supplied plan.
6. MIR/regalloc consume roles/classes/groups/ties/early-clobbers/clobber units
   from typed prepared/MIR records only.
7. Late assembly substitutes completed assignments and first parses/encodes
   the opaque instruction payload.

## Execution Rules

- Step 1 is a design repair and joint architecture review. No later
  implementation step is authorized until its completion check passes.
- Keep Step 1 bounded to the inline-asm checkpoint and its CanonicalBir,
  `PreparedBir` publication, and new-MIR adjacency. Do not take over review of unrelated
  scaffold areas; only confirm the architecture-wide acceptance prerequisite.
- Treat the user-authored `src/backend/bir/**` scaffold as authoritative input; do not
  modify it in the checkpoint-repair packet.
- Use the scaffold stage order exactly. Each output must name its verifier gate
  and revision/staleness behavior.
- Keep the RV64 preparation table evidence-backed: `r`, `VR`, `VRM2`, `VRM4`,
  and `VRM8`; reject `VRM1` and unreviewed syntax.
- Prove payload equality independently from constraint-plan correctness.
- Require negative proofs for stale/mismatched plans, illegal groups, ties,
  early-clobbers, clobbers, and unsupported syntax.
- Keep legacy translation units absent from compile metadata.
- Treat the missing authoritative new-MIR ownership/stage-token/verifier
  contract as a Step 1 blocker. Do not recreate deleted
  `src/backend/bir/mir/**` documents or choose a replacement filesystem owner
  within this runbook.
- Record the dangling MIR links in the surviving BIR README/pipeline as
  scaffold adjacency debt outside this packet; they are not implementation
  authority.

## Step 1: Repair PreparedBir publication and define the missing new-MIR handoff

Goal: reconcile the inline-asm checkpoint with `PreparedBir` publication and
freeze the bounded decisions required from an authoritative new-MIR contract
before any implementation is allowed.

Primary targets:

- `docs/inline_asm_transport/`
- the surviving ordered pipeline, verification, and preparation contracts
- the missing bounded `PreparedBir` to new-MIR adjacency contract

Concrete actions:

- Remove LIR-to-BIR as constraint normalizer and remove target-contract
  provenance, physical-register interpretation, normalized register classes,
  and group requirements from canonical BIR schema.
- Specify target-independent Raw/Canonical BIR inline asm as opaque payload,
  structured source operands, original constraint/clobber syntax, results,
  and writebacks only.
- Specify `preparation/inline_asm` as sole target-aware normalizer consuming
  immutable `CanonicalBir` plus target context and producing a typed immutable
  plan bound to exact module/function revision and target-context identity.
- Place roles, classes, group widths/alignment/contiguity, ties,
  early-clobbers, memory/CC effects, and physical clobber units in that plan.
- Specify the plan as a typed component of the verified `PreparedBir` product,
  bound to the same canonical revision and target-context identity/version.
- Require Prepared-input verification before `PreparedBir` publication and
  reject stale, missing, mismatched, or cross-bundle plan components there.
- Require MIR construction to consume verified `PreparedBir` or a verified
  read-only view; prohibit raw `CanonicalBir` plus a side plan.
- Record the exact still-missing new-MIR decisions: MIR owner, constructed-MIR
  output stage token, immediate verifier profile/gate, revision/staleness
  behavior, diagnostics, and transactional failure/publication behavior.
- Keep the location and shape of that authoritative new-MIR contract open for
  an explicit architecture decision; do not recreate deleted BIR-owned MIR
  documents.
- Re-home all related diagnostics and proof rows to the stage that can decide
  them.
- Preserve the evidence-backed RV64 `r`, `VR`, `VRM2`, `VRM4`, `VRM8` table as
  a preparation contract and keep `VRM1` unsupported.
- Review the repaired checkpoint against CanonicalBir, preparation,
  Prepared-input verification, `PreparedBir` publication, and the newly
  authoritative following-MIR contract using
  `src/backend/bir/REVIEW_TEMPLATE.md`.
- Record architecture-wide acceptance as an external prerequisite rather than
  expanding this packet into review of unrelated scaffold areas.
- Do not declare implementation authorization until that prerequisite holds
  and this checkpoint is accepted against the ordered contracts. The earlier
  checkpoint re-review is superseded by this ownership change.

Completion check:

- The checkpoint routes the revision/target-bound inline-asm plan through
  verified `PreparedBir`; an authoritative new-MIR contract supplies the
  missing owner, stage token, immediate verifier, staleness, diagnostics, and
  transactional publication decisions; and the bounded adjacency review
  accepts the complete handoff. Step 2 remains unauthorized until this check
  and the separate architecture-wide acceptance prerequisite both pass.

## Step 2: Establish target-independent transport through Canonical BIR

Goal: publish source-semantic inline asm without target normalization.

Concrete actions:

- Introduce the smallest structured parser/HIR/LIR carrier for operand order,
  values, destinations, results, original constraints, and original clobbers;
  keep LLVM compatibility rendering separate and non-authoritative.
- Preserve payload bytes exactly and remove all early mnemonic, placeholder,
  directive, and `.insn` parsing from the new-backend route.
- Add Raw/Canonical BIR node, builders, views, verification, results, and
  writebacks containing only target-independent facts.
- Add direct transport tests for multi-output, distinct `UseDef` identities,
  original syntax retention, malformed payload byte equality, and absence of
  target-normalized fields.

Completion check:

- Published `CanonicalBir` retains exact source facts and payload without
  target classification or instruction parsing.

## Step 3: Implement target-aware inline-asm preparation

Goal: turn source constraints into a verified, revision-bound typed plan.

Concrete actions:

- Define target context and plan identities, exact canonical revision binding,
  staleness checks, immutable APIs, and transactional failure behavior.
- Normalize the reviewed RV64 scalar/vector roles, groups, ties,
  early-clobbers, effects, and clobber units only in
  `preparation/inline_asm`.
- Reject `VRM1`, alternatives, unsupported classes/aliases, malformed ties,
  type mismatches, and clobbers with stable preparation-owned diagnostics.
- Add preparation tests for all admitted forms plus unsupported, stale, and
  target-mismatch cases; prove `CanonicalBir` remains unchanged.

Completion check:

- A verified plan contains every target-specific fact required downstream,
  and neither MIR nor regalloc needs source-string interpretation.

## Step 4: Construct MIR from verified PreparedBir

Goal: translate the verified prepared product into allocatable machine
requirements.

Concrete actions:

- Implement the separately accepted new-MIR ownership/identity contract for
  inline-asm pseudos and require verified `PreparedBir` (or its verified
  read-only view) at the construction boundary.
- Preserve operand/result order, distinct incoming/produced identities, ties,
  clobbers, and opaque payload while translating plan facts into MIR records.
- Reject absent, stale, mismatched, or unrepresentable plans before partial MIR
  publication.
- Add direct PreparedBir-to-MIR tests, including byte equality, cross-bundle,
  stale-revision, target-mismatch, and verifier-gate failures.

Completion check:

- MIR carries complete structured allocation requirements and opaque payload
  without legacy/prealloc code or constraint-string parsing.

## Step 5: Enforce inline-asm requirements in regalloc

Goal: make the prepared requirements causally determine legal assignments.

Concrete actions:

- Enforce register class, reserved units, group width/alignment/contiguity,
  ties, liveness/interference, early-clobbers, and explicit clobber exclusion.
- Allocate vector groups atomically and use the same requirement-aware filter
  in normal, eviction, retry, spill, and fallback paths.
- Fail explicitly when no legal assignment exists; never weaken a requirement.
- Add positive and negative invariant tests for scalar and every admitted
  vector group.

Completion check:

- Changing a structured requirement changes allocation behavior, and illegal
  overlaps/groups fail by general invariants rather than testcase spellings.

## Step 6: Add the late assembler seam

Goal: make late assembly the first instruction/template parser.

Concrete actions:

- Accept opaque payload plus completed physical operand assignments only.
- Substitute the reviewed placeholder grammar, then parse mnemonics,
  directives, `.insn`, and encoding details.
- Keep constraints, virtual registers, and allocation policy out of the API.
- Prove invalid mnemonic and malformed `.insn` payloads survive every earlier
  boundary unchanged and fail here.

Completion check:

- Diagnostics and instrumentation identify late assembly as the first parser,
  and encoding requires completed allocation.

## Step 7: Prove the bounded end-to-end route

Goal: accept the route only when all typed boundaries work together.

Concrete actions:

- Run scalar input/output/read-write and every admitted RV64 vector group
  through transport, preparation, MIR, allocation, and late assembly.
- Verify payload equality, preparation-plan revision matching, legal allocated
  units, substitution, and owner-specific negative diagnostics.
- Confirm build metadata excludes legacy/prealloc/old-MIR sources and run the
  supervisor-selected broader regression guard.

Completion check:

- The bounded route is green, failures occur at their sole owners, and review
  finds no early parsing, canonical target leakage, string-driven regalloc, or
  testcase overfit.
