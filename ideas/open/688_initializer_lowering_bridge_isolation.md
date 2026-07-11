# Initializer Lowering Bridge Isolation

Status: Open
Type: Implementation idea
Order: 4 of 6 in the `LIR -> BIR` adapter boundary first wave
After: `ideas/open/687_structured_layout_bridge_isolation.md`
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
First Owning Layer: Initializer bridge
Consumes:
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
Related Evidence:
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

## Goal

Isolate global and aggregate initializer lowering as an adapter-owned bridge
from LIR spelling into semantic BIR facts, without changing prepared object
data plans, relocation spelling, emitted data layout, tests, expectations, or
runtime behavior.

## Why This Exists

The handoff docs classify global declarations, link-name resolution, string
constants, scalar/byte string/array/aggregate/pointer initializer lowering,
pointer initializer offsets, and known global addresses as an initializer
bridge. Keeping that bridge separate prevents initializer compatibility from
leaking into canonical BIR, prepared object-data, or target emission policy.

## In Scope

- Own `src/backend/bir/lir_to_bir/globals.cpp`,
  `src/backend/bir/lir_to_bir/global_initializers.cpp`, root string constant
  rewrite helpers, initializer value materialization, `GlobalTypes`,
  `FunctionSymbolSet`, and known global-address import paths.
- Hide textual initializer compatibility and global declaration import details
  behind narrower adapter-owned contracts.
- Preserve existing BIR output, prepared object data, emitted data layout,
  diagnostics, and runtime behavior.

## Out Of Scope

- Prepared object-data plans, RV64 relocation spelling, target data emission,
  canonical BIR route schema changes, memory/provenance cleanup, call ABI
  cleanup, MIR consumers, tests, expectations, unsupported markers, allowlists,
  runtime behavior, and harness policy.
- Changing initializer semantics as part of bridge isolation unless a future
  source idea explicitly owns semantic repair.

## Behavior-Preserving Proof Surface

Use compile proof plus existing global/initializer adapter coverage selected by
the active runbook. If the slice touches emitted global data behavior or
prepared object-data paths, escalate proof and split the downstream change into
a separate idea when needed.

## Acceptance Criteria

- Initializer lowering compatibility state is narrower or more private within
  the adapter.
- `GlobalTypes`, `FunctionSymbolSet`, string constant rewrite state, and
  initializer value materialization remain import-local unless a later idea
  proves a stable public contract.
- Prepared object-data and target data emission behavior is unchanged.
- Proof is recorded in `todo.md`.

## Reviewer Reject Signals

- Reject changing relocation spelling, emitted global-data layout, prepared
  object-data plans, target data emission, tests, expectations, unsupported
  markers, allowlists, or runtime behavior in this idea.
- Reject testcase-shaped initializer parsing, named-case-only rewrites, or
  expectation downgrades claimed as bridge isolation.
- Reject moving raw initializer spelling compatibility into public BIR or
  prepared/prealloc ownership.
- Reject helper renames or classification-only changes that leave the same
  initializer bridge coupling exposed.
- Reject proof that omits global or initializer coverage when those paths were
  touched.
