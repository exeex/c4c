# Structured Layout Bridge Isolation

Status: Open
Type: Implementation idea
Order: 3 of 6 in the `LIR -> BIR` adapter boundary first wave
After: `ideas/open/686_private_detail_header_contraction.md`
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
First Owning Layer: Structured type/layout bridge
Consumes:
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
Related Evidence:
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`

## Goal

Isolate the structured type and layout bridge inside the `LIR -> BIR` adapter
so legacy type text parsing, type declarations, typed operand parsing, and
layout fallback maps are import-local contracts rather than public BIR,
prepared/prealloc, or target policy.

## Why This Exists

The handoff docs classify structured type/layout work as a bridge layer between
raw LIR spelling and stable BIR facts. This bridge should be clarified before
initializer, memory/provenance, call ABI, BIR semantic model, or prepared
handoff changes rely on structured layout behavior.

## In Scope

- Own `src/backend/bir/lir_to_bir/types.cpp`,
  `src/backend/bir/lir_to_bir/aggregate.cpp`,
  `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`, structured layout
  fallback maps, typed operand parsing, `TypeDeclMap`, and related
  import-local aggregate layout lookup state.
- Keep legacy type text and structured layout compatibility hidden behind
  adapter-private contracts.
- Preserve existing BIR output and downstream observable behavior.
- Use `memory_helpers.hpp` only as a narrow pure layout/projection helper
  precedent, not as a destination for stateful lowerer policy.

## Out Of Scope

- Changing canonical BIR type/model authority, public BIR route schemas,
  target aggregate transport lanes, byval ABI placement, prepared storage
  layout, MIR consumers, tests, expectations, unsupported markers, allowlists,
  runtime behavior, or harness policy.
- Combining structured layout cleanup with initializer, memory/provenance, call
  ABI, BIR semantic model, or prepared/prealloc changes.

## Behavior-Preserving Proof Surface

Use compile proof plus focused existing type/layout adapter tests or a
dump-equivalence proof selected by the active runbook. Escalate only if shared
type/model or downstream layout files are touched.

## Acceptance Criteria

- Structured type/layout compatibility state is isolated behind a narrower
  adapter-owned contract.
- `TypeDeclMap`, structured layout fallback maps, and aggregate layout lookups
  remain import-local unless a later source idea proves a target-neutral BIR
  contract.
- Existing BIR, MIR, object, and runtime behavior remains unchanged.
- Proof is recorded in `todo.md`.

## Reviewer Reject Signals

- Reject raw LIR type spelling maps moved into public BIR, prepared/prealloc,
  target, or MIR ownership.
- Reject target-specific aggregate transport, byval ABI, or storage layout
  policy changes inside this bridge idea.
- Reject testcase-shaped layout matching, named-case-only fixes, expectation
  rewrites, unsupported downgrades, allowlist filtering, or weaker proof.
- Reject helper renames or classification-only edits claimed as layout bridge
  isolation.
- Reject retaining the same fallback behavior behind a new abstraction without
  reducing the adapter boundary.
