Status: Active
Source Idea Path: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Pointer-Base-Plus-Offset Consumers

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: audited pointer-base-plus-offset producers,
support facts, shared prepared/prealloc exposure points, and narrow RV64,
AArch64, and x86 consumers.

Audited producer/support surfaces:

- `src/backend/prealloc/regalloc/value_homes.cpp` produces
  `PreparedValueHomeKind::PointerBasePlusOffset` from semantic pointer carrier
  authority, carrying result identity, base value, optional base symbol, byte
  delta, and any target placement metadata.
- `src/backend/prealloc/value_locations.hpp` defines
  `PreparedPointerBasePlusOffsetFact`; `as_pointer_base_plus_offset_fact(...)`
  accepts only the pointer-base-plus-offset home shape with valid function,
  result value, base value, delta, and no immediate payloads. Its direct-copy
  and signed-12-bit flags are support/range facts only.
- `src/backend/prealloc/prepared_contract_verifier.*` verifies structural
  coherence only: missing home/function/result/base/delta and conflicting
  home/payload fail closed, but a coherent contract is not selected
  pointer-arithmetic authority.
- `src/backend/prealloc/decoded_home_storage.*` exposes the decoded kind as
  `PointerBasePlusOffset` while leaving it `UnsupportedValueHomeKind`.
- `src/backend/prealloc/prepared_lookups.cpp` maps the home to
  `PreparedMoveStorageKind::None`, so move-storage lookup does not authorize
  it as a normal register/stack move source.
- `src/backend/prealloc/publication_plans.*` classifies scalar publication as
  hook kind `PointerBasePlusOffset` and storage encoding `ComputedAddress`,
  and records source/base/delta fields in
  `plan_prepared_store_source_publication(...)`; these are publication
  support facts, not selected-use authority.
- `src/backend/prealloc/prepared_printer/*`, `formal_publications.cpp`,
  `storage_plans.cpp`, `call_plans.cpp`, and prepared object traversal expose
  or classify computed-address/source-delta fields for diagnostics, dumps,
  call routing, or storage classification. They do not own freshness.

Audited consumer classification:

- Representative shared prepared/prealloc candidate:
  `plan_prepared_store_source_publication(...)` for a
  `PointerBasePlusOffset` source home, currently consumed by AArch64
  `plan_pointer_base_plus_offset_store_local_publication(...)` /
  `lower_pointer_base_plus_offset_store_local_publication(...)`. This route
  already has the source home, source value identity, base/delta fields,
  destination access, producer metadata, and existing freshness plumbing on the
  plan record, so it is the narrowest shared place to require selected
  pointer-arithmetic authority before target materialization accepts the
  computed pointer.
- Target-consume-only/deferred RV64 paths:
  `prepared_edge_publication_emit.cpp` consumes a structurally coherent
  pointer-base-plus-offset source by finding a register home for the base and
  emitting the delta; `prepared_scalar_emit.cpp`,
  `prepared_emit_context.cpp`, `prepared_frame_emit.cpp`, and
  `object_emission.cpp` consume or dump register/stack/address materialization
  details. These should remain out of scope for this runbook except as later
  consumers of shared selected authority.
- Target-consume-only/deferred AArch64 paths:
  `operands.cpp` rejects decoded pointer-base-plus-offset operands;
  `memory.cpp` materializes computed pointer addresses for store-local
  publication; `calls.cpp` uses computed-address argument/source metadata for
  aggregate/address call copies. The memory route is the target-side consumer
  of the selected representative shared plan; broad operand/call migration is
  out of scope.
- x86 paths are rejection/deferred paths:
  `module.cpp` rejects pointer-base-plus-offset for prepared i32 return homes,
  compare branch entry homes, and prepared call result homes. x86 publication
  plan reuse tests exercise shared planning data, not target semantic
  acceptance.

## Suggested Next

Execute Step 2 from `plan.md`: define the selected pointer-arithmetic
ownership contract for the store-source publication route. The contract should
state the exact freshness use/source/proof/rank and reference dimensions that
authorize accepting a `PointerBasePlusOffset` source home for store-local
publication.

## Watchouts

- Keep pointer-value indirect memory-use freshness in idea 600.
- Do not treat home shape, byte delta, range/layout facts, stack/register
  placement, target offset encodability, target operand shape, diagnostics, or
  dumps as selected pointer-arithmetic authority.
- The selected route should be shared-prealloc first:
  `PreparedStoreSourcePublicationPlan` / `plan_prepared_store_source_publication(...)`.
  AArch64 memory lowering can consume that result, but target-local
  materialization must not become the semantic authority.
- Target paths explicitly out of scope for this runbook: RV64 edge publication,
  scalar emit, frame/context helpers, and object-emission diagnostics; AArch64
  generic operand resolution, call lowering, and broad memory lowering beyond
  the representative store-local consumer; x86 module lowering/rejections;
  semantic GEP target consumption; relocation/materialization semantics.
- Do not reuse branch, edge-publication, move-bundle, select-carrier, alias, or
  pointer-value memory-use freshness vocabulary unless Step 2 proves the
  ownership boundary is identical. The audit suggests pointer arithmetic needs
  its own narrow selected-authority vocabulary.

## Proof

Audit-only/todo-only packet. No build or tests were run, and `test_after.log`
was not updated. Proof command: `git diff --check`.
