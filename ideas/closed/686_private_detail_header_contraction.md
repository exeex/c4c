# Private Detail Header Contraction

Status: Closed
Type: Implementation idea
Order: 2 of 6 in the `LIR -> BIR` adapter boundary first wave
After: `ideas/open/685_lir_import_context_extraction.md`
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
First Owning Layer: LIR import
Consumes:
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
Related Evidence:
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`

## Goal

Contract `src/backend/bir/lir_to_bir/lowering.hpp` and adjacent private helper
boundaries so split translation units see only the declarations they need,
without changing adapter behavior.

## Why This Exists

The Step 3 handoff classifies `lowering.hpp` as a broad private adapter
declaration surface, not a public BIR model boundary. After the import context
has a clearer shape, the next behavior-preserving cleanup should reduce
private header width so later family-specific work can isolate structured
layout, initializer, memory/provenance, and call ABI bridges without depending
on one large detail header.

## In Scope

- Own `src/backend/bir/lir_to_bir/lowering.hpp` and private adapter helper
  declarations needed by split implementation files under
  `src/backend/bir/lir_to_bir/`.
- Move or hide private declarations when an existing translation unit can own
  them locally.
- Prefer narrow helper headers only when they remove real cross-TU coupling.
- Preserve the import-local nature of `ValueMap`, `GlobalTypes`,
  `TypeDeclMap`, `FunctionSymbolSet`, CFG/phi scratch maps, and memory side
  tables.

## Out Of Scope

- Public BIR route schemas, public query surfaces, route records, printer,
  validator, prepared/prealloc products, target emission, MIR consumers, tests,
  expectations, unsupported markers, allowlists, and runtime behavior.
- Semantic lowering changes for structured types, initializers, memory,
  provenance, calls, or ABI classification.
- Moving public BIR model records into `src/backend/bir/lir_to_bir/`.

## Behavior-Preserving Proof Surface

Use compile-only proof or a focused BIR adapter test proof selected by the
active runbook. The proof should show that declarations were hidden, narrowed,
or moved without behavior changes. Escalate only if a shared non-adapter header
is changed.

## Acceptance Criteria

- `lowering.hpp` exposes fewer unrelated responsibilities or a clearer private
  boundary than before.
- Any new helper boundary is narrower than the old detail surface and has a
  concrete ownership reason.
- No downstream layer starts depending on adapter-private state.
- The slice remains behavior-preserving and records proof in `todo.md`.

## Closure Note

Closed after contracting the private adapter detail surface in two
behavior-preserving slices:

- `FunctionSymbolSet` method bodies moved out of `lowering.hpp`.
- `is_known_function_link_name_id` was removed from `lowering.hpp` and made
  internal to `globals.cpp`.

The remaining cross-translation-unit `is_known_function_global_address`
declaration still has real adapter-internal consumers and stays declared in
`lowering.hpp`. `todo.md` recorded backend proof for the final slice:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`, passing 302/302 backend
tests. The supervisor also reported a matching backend regression guard pass
with before=302 failed=0 total=302 and after=302 failed=0 total=302 using
`--allow-non-decreasing-passed`.

## Reviewer Reject Signals

- Reject a rename-only reshuffle that leaves the exact old broad responsibility
  pile under a new file or type name.
- Reject moving raw LIR spelling maps or adapter-private state into public BIR,
  prepared/prealloc, target, or MIR ownership.
- Reject public BIR route schema changes, prepared publication changes, target
  codegen changes, tests or expectation rewrites, unsupported downgrades,
  allowlist filtering, or runtime changes.
- Reject family-specific semantic repair hidden inside header contraction.
- Reject proof that does not compile the affected adapter translation units.
