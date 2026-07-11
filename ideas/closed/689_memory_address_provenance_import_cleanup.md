# Memory Address Provenance Import Cleanup

Status: Closed
Type: Implementation idea
Order: 5 of 6 in the `LIR -> BIR` adapter boundary first wave
After: `ideas/open/688_initializer_lowering_bridge_isolation.md`
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
First Owning Layer: Memory/address provenance import
Consumes:
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
Related Evidence:
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

## Goal

Clean up memory and address provenance import boundaries so LIR alloca,
load/store, GEP, intrinsic, pointer slot, dynamic array, pointer-address, and
formal provenance facts are imported into semantic BIR without leaking
route-local side tables into public BIR, prepared/prealloc, target, or MIR
ownership.

## Why This Exists

The umbrella handoff identifies memory/address provenance as a distinct import
layer. It populates BIR memory/address records, but it does not own public BIR
Route 3 authority, prepared frame or storage policy, target addressing
legality, or MIR memory emission. This cleanup should narrow adapter-local side
tables before downstream memory or prepared storage changes are attempted.

## In Scope

- Own `src/backend/bir/lir_to_bir/memory/*.cpp`,
  `src/backend/bir/lir_to_bir/memory/memory_types.hpp`,
  `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`, local
  pointer/slot/provenance side tables, pointer-address records, dynamic
  aggregate/pointer arrays, local aggregate slots, and formal pointer
  provenance publication during import.
- Keep `LocalSlotTypes`, `LocalPointerSlots`,
  `LocalIndirectPointerSlotSet`, and memory side tables import-local.
- Preserve public BIR memory route behavior and downstream prepared/target
  behavior.

## Out Of Scope

- Moving public BIR Route 3 authority into private lowering.
- Editing prepared frame/storage policy, target addressing legality, MIR memory
  emission, canonical BIR route schemas, tests, expectations, unsupported
  markers, allowlists, runtime behavior, or harness policy.
- Combining memory/provenance import cleanup with structured layout,
  initializer, call ABI, BIR semantic model, or prepared/prealloc rewrites.

## Behavior-Preserving Proof Surface

Use compile proof plus narrow memory/provenance adapter coverage selected by
the active runbook. Escalate only if the future packet edits shared
memory/address model files or downstream prepared/target surfaces.

## Acceptance Criteria

- Memory/provenance side tables are narrower, better isolated, or clearer as
  adapter-owned import state.
- Public BIR Route 3 records and query surfaces remain canonical BIR authority.
- Prepared frame/storage policy, target addressing legality, MIR memory
  emission, and runtime behavior remain unchanged.
- Proof is recorded in `todo.md`.

## Closure Summary

Closed on 2026-07-11 after the active runbook completed Step 4 final boundary
audit. The completed route narrowed the lowerer re-export surface for the
reviewed memory/provenance side-table families and left those families as
memory-owned adapter import state in
`src/backend/bir/lir_to_bir/memory/memory_types.hpp`.

No remaining follow-up was found inside the reviewed lowerer re-export
alias-contraction route. Public BIR Route 3 authority, prepared/prealloc,
target, MIR, runtime behavior, tests, expectations, unsupported markers, and
allowlists were preserved.

Close proof used:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)' | tee test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

The close proof selected 303 tests, passed 303, failed 0, and the regression
guard reported no new failing tests.

## Reviewer Reject Signals

- Reject moving BIR Route 3 public authority into private lowering or moving
  adapter side tables into public BIR/prepared/target/MIR ownership.
- Reject target addressing, prepared storage, MIR emission, runtime, tests,
  expectations, unsupported-marker, or allowlist changes in this idea.
- Reject testcase-shaped memory or provenance shortcuts, named-case-only
  repairs, expectation rewrites, or weaker proof commands.
- Reject helper renames or classification-only edits that keep the same
  memory side-table coupling exposed.
- Reject proof that does not cover touched memory/provenance adapter paths.
