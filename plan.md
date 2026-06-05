# BIR Prealloc Legacy Compatibility Residue Audit Runbook

Status: Active
Source Idea: ideas/open/109_bir_prealloc_legacy_compatibility_residue_audit.md

## Purpose

Audit retained BIR/prealloc compatibility and deferred paths before x86/RISC-V
backend rebuild work depends on them.

Goal: classify each known residue as safe, stale, contract-worthy, visibility
worthy, architecture-risky, or deserving a narrow follow-up idea.

Core Rule: this is analysis-only. Do not implement cleanup or change backend,
BIR, prealloc, or test expectations while executing this plan.

## Read First

- `ideas/open/109_bir_prealloc_legacy_compatibility_residue_audit.md`
- `ideas/closed/100_aarch64_00204_stdarg_hfa_runtime_repair.md`
- `ideas/closed/101_closure_note_followup_recovery_audit.md`
- `ideas/closed/104_bir_prealloc_pointer_carrier_provenance_contract.md`
- `ideas/closed/106_prealloc_stack_layout_slice_family_fact_contract.md`
- `ideas/closed/107_prealloc_inline_asm_memory_effect_metadata_contract.md`
- `ideas/closed/108_prepared_select_chain_dump_contract_coverage.md`

## Current Scope

Audit/read surfaces:

- `src/backend/bir/**`
- `src/backend/prealloc/**`
- `src/backend/mir/aarch64/codegen/**` only as consumer examples

Allowed execution writes:

- `todo.md` packet progress
- new focused `ideas/open/*.md` follow-up ideas
- source-idea close notes when lifecycle closure is requested

## Non-Goals

- Do not implement residue cleanup.
- Do not move stack placement, register allocation, frame offsets, or target ABI
  placement into BIR.
- Do not reopen ideas `100` through `108`.
- Do not create direct x86/RISC-V backend implementation work.
- Do not classify every `prepared_lookups.cpp` helper as duplicate authority
  without proving it derives semantic facts.

## Classification Labels

Use these labels exactly when recording dispositions:

- `safe-compatibility-glue`
- `needs-explicit-bir-fact`
- `needs-prealloc-contract-test`
- `needs-prepared-dump-visibility`
- `arch-consumer-risk`
- `stale-no-action`
- `needs-follow-up-idea`

## Execution Rules

- Keep evidence tied to named functions, helpers, files, tests, or closed-idea
  notes.
- If a residue needs implementation, create a narrow follow-up idea instead of
  changing code.
- Every new follow-up idea must include exact ownership, proof route, and
  reviewer reject signals.
- Treat broad "clean BIR/prealloc" proposals as route drift.
- Use `rg`/read-only inspection first; do not run backend validation unless a
  file outside lifecycle artifacts is accidentally changed.

## Ordered Steps

### Step 1: Reconstruct Retained Residue Inventory

Goal: extract the compatibility and deferred paths explicitly retained by the
closed idea notes.

Primary targets:

- `ideas/closed/100_aarch64_00204_stdarg_hfa_runtime_repair.md`
- `ideas/closed/101_closure_note_followup_recovery_audit.md`
- `ideas/closed/104_bir_prealloc_pointer_carrier_provenance_contract.md`
- `ideas/closed/106_prealloc_stack_layout_slice_family_fact_contract.md`
- `ideas/closed/107_prealloc_inline_asm_memory_effect_metadata_contract.md`
- `ideas/closed/108_prepared_select_chain_dump_contract_coverage.md`

Actions:

- Inspect each closed note for retained compatibility, deferred no-action
  items, and explicit future-risk comments.
- Seed a residue table in `todo.md` with item name, origin note, current
  suspected owner, and evidence needed.
- Include at least the source idea's named residues:
  `LegacySlotNameSliceFamilyCompatibility`,
  `LegacyFrameAddressNameCompatibility`, pointer-carrier and memory-base
  publication/dereference boundary residue, deferred store-source dump
  visibility, dynamic alloca / VLA no-action notes, and
  `prepared_lookups.cpp` helper authority questions.

Completion check:

- `todo.md` contains a residue inventory with closed-note citations and no
  implementation edits exist.

### Step 2: Verify Current Reachability And Ownership

Goal: determine whether each inventoried residue is live, stale, harmless glue,
or evidence of missing shared authority.

Primary targets:

- `src/backend/bir/**`
- `src/backend/prealloc/**`
- `src/backend/mir/aarch64/codegen/**` as consumer examples only

Actions:

- Trace each residue from producer facts through prealloc and AArch64 consumer
  reads.
- Mark unreachable or superseded paths as `stale-no-action` only when the code
  evidence supports it.
- Mark compatibility wrappers as `safe-compatibility-glue` only when they do
  not own semantic facts that BIR/prealloc should publish.
- Identify places where architecture consumers infer facts from names, helper
  shape, or local reconstruction instead of shared BIR/prealloc facts.

Completion check:

- Every inventory row has reachability evidence and a provisional
  classification.

### Step 3: Draft Focused Follow-Up Ideas

Goal: turn only actionable residues into narrow source ideas with clear proof
routes.

Actions:

- For every residue classified `needs-follow-up-idea`, create one focused
  `ideas/open/*.md` file.
- Include goal, why it exists, in-scope work, out-of-scope work, acceptance or
  completion criteria, and `## Reviewer Reject Signals`.
- Prefer source-fact or contract-test ownership over architecture-specific
  consumer patches unless the evidence shows the residue is truly consumer
  local.
- Do not bundle unrelated residues into one broad cleanup idea.

Completion check:

- Each actionable residue has a focused open idea, and each created idea has
  concrete reject signals against testcase-shaped shortcuts and expectation
  downgrades.

### Step 4: Prepare Audit Closure Evidence

Goal: make the source idea closure decision mechanically reviewable.

Actions:

- Consolidate the final disposition table in `todo.md`, including every
  audited residue and its final label.
- List every new follow-up idea path beside the residue it covers.
- If no follow-up idea was created for a retained path, record why it is safe,
  stale, or out of scope.
- Confirm no implementation files changed.

Completion check:

- The audit evidence is complete enough for the plan owner to close the source
  idea with durable notes or report the exact remaining ambiguity.

