# Memory Address Provenance Import Cleanup Runbook

Status: Active
Source Idea: ideas/open/689_memory_address_provenance_import_cleanup.md

## Purpose

Activate the fifth first-wave `LIR -> BIR` adapter boundary cleanup from the
umbrella handoff: memory/address provenance import cleanup.

Goal: narrow or clarify adapter-owned memory/provenance import state while
preserving public BIR memory route authority, prepared storage policy, target
addressing legality, MIR emission, object output, and runtime behavior.

## Core Rule

Keep this behavior-preserving and adapter-local. Do not move public BIR Route 3
authority into private lowering, and do not move adapter-local side tables into
public BIR, prepared/prealloc, target, or MIR ownership.

## Read First

- `ideas/open/689_memory_address_provenance_import_cleanup.md`
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`

## Current Targets

Primary owned implementation surfaces:

- `src/backend/bir/lir_to_bir/memory/*.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- adapter-local pointer, slot, provenance, pointer-address, dynamic aggregate,
  dynamic pointer-array, local aggregate-slot, and formal-provenance import
  state

Allowed supporting reads:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/*.cpp`
- canonical BIR memory route declarations and query surfaces when needed to
  verify boundary preservation

## Non-Goals

- Do not edit prepared frame or storage policy.
- Do not edit target addressing legality, MIR memory emission, object data, or
  runtime behavior.
- Do not edit tests, expectations, unsupported markers, allowlists, runtime
  harness policy, or baseline acceptance policy.
- Do not combine this cleanup with structured layout, initializer, call ABI,
  BIR semantic model, prepared/prealloc, or target rewrites.
- Do not claim progress through helper renames or classification-only edits
  that leave the same memory side-table coupling exposed.

## Working Model

The memory/address provenance importer may populate canonical BIR memory and
address records, but its raw LIR compatibility state is private adapter state.
`LocalSlotTypes`, `LocalPointerSlots`, `LocalIndirectPointerSlotSet`, pointer
address records, dynamic arrays, local aggregate slots, and formal provenance
publication should remain implementation details unless a later source idea
promotes a narrower public contract.

## Execution Rules

- Keep each step small enough for a fresh build plus focused backend or adapter
  proof.
- If a packet needs to edit public BIR route schemas, prepared/prealloc,
  target, MIR, tests, expectations, unsupported markers, allowlists, or runtime
  behavior, stop and route that work to a separate idea.
- Preserve existing diagnostics and BIR output unless a changed diagnostic is
  an unavoidable result of removing an adapter-local leak; record that in
  `todo.md` before proof.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different artifact.

## Steps

### Step 1 - Inventory Memory Import Coupling

Goal: identify the current adapter-local memory/provenance state and the
specific coupling that can be narrowed first.

Primary target:

- `src/backend/bir/lir_to_bir/memory/`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- relevant declarations in `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Inspect local slot, pointer slot, indirect pointer slot, pointer address,
  dynamic aggregate, dynamic pointer-array, local aggregate-slot, and formal
  provenance state.
- Separate private import scratch state from canonical BIR memory route
  records and public query surfaces.
- Choose one first behavior-preserving narrowing target that can be proved
  without downstream edits.
- Record the selected first target and proof command in `todo.md`.

Completion check:

- `todo.md` names the first owned memory/provenance narrowing packet.
- No source idea, downstream ownership, tests, expectations, or runtime policy
  changed.

### Step 2 - Narrow One Adapter-Local Memory State Boundary

Goal: make one memory/provenance side-table boundary narrower or clearer
inside the adapter.

Actions:

- Move or hide the selected side-table access behind memory-owned helpers or
  tighter local declarations when that reduces cross-file coupling.
- Keep canonical BIR memory route records and query APIs unchanged.
- Preserve all prepared, target, MIR, object, diagnostic, and runtime behavior.
- Avoid broad lowerer rewrites; touch only the files needed for the selected
  state boundary.

Completion check:

- The selected adapter-local state has a narrower ownership boundary.
- Public BIR, prepared/prealloc, target, MIR, tests, expectations, unsupported
  markers, allowlists, and runtime behavior are unchanged.
- Fresh build and focused proof pass.

### Step 3 - Continue Pointer And Provenance Import Isolation

Goal: repeat the narrowing pattern for the next pointer/address/provenance
state family only after Step 2 has a clean proof.

Actions:

- Select the next state family from pointer slots, indirect pointer slots,
  pointer-address records, local aggregate slots, dynamic aggregate or pointer
  arrays, or formal pointer provenance.
- Apply the smallest behavior-preserving isolation that keeps state
  adapter-local.
- Stop if the work requires prepared storage, target addressing, or MIR memory
  emission changes.

Completion check:

- A second adapter-local memory/provenance boundary is narrower or explicitly
  documented as already isolated.
- Fresh build and focused memory/provenance proof pass.

### Step 4 - Final Boundary Audit And Proof

Goal: confirm the active idea is satisfied or identify the precise remaining
memory/provenance cleanup follow-up.

Actions:

- Audit the diff against the source idea and handoff docs.
- Confirm that public BIR Route 3 authority and downstream prepared/target/MIR
  behavior stayed unchanged.
- Run the supervisor-selected acceptance proof; prefer build plus focused
  backend coverage, and escalate only if shared memory/address model files or
  downstream surfaces were touched.
- Record proof and any remaining follow-up in `todo.md`.

Completion check:

- Acceptance criteria from the source idea are either satisfied or a precise
  remaining follow-up is recorded for lifecycle review.
- No testcase-shaped shortcut, expectation rewrite, unsupported downgrade,
  allowlist edit, or weaker proof command is used as progress.
