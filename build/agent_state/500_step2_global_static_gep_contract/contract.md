# 500 Step 2 Global/Static GEP Contract Route

Step 2 audited the Step 1 row evidence and existing global/static authority
surfaces. The final semantic global/static GEP admission contract is not ready
to implement directly because the current production surfaces do not publish a
durable lower certificate tying a GEP candidate to one global/static source
object, one layout path, one byte range, one dynamic-index/range authority, and
one LIR producer coordinate.

## Inputs

- Step 1 classification:
  `build/agent_state/500_step1_global_static_gep_classification/summary.md`
- Direct candidate rows:
  `src/20000717-4.c`, `src/20031214-1.c`, `src/20080424-1.c`, and
  `src/20120808-1.c`
- Boundary rows:
  `src/20051104-1.c` and `src/ieee/copysign2.c`
- Existing surfaces read for authority shape:
  `bir::Global`, `bir::LoadGlobalInst`, `bir::StoreGlobalInst`,
  `bir::MemoryAccessProvenance`, `BirFunctionLowerer::GlobalAddress`,
  `resolve_global_gep_address`, `resolve_relative_global_gep_address`,
  `resolve_global_dynamic_*_array_access`, `PreparedMemoryAccess`, and
  `PreparedGlobalObjectData`.

## Existing Surfaces

- `bir::Global` carries module-level global identity, `LinkNameId`, value type,
  scalar/integer-array layout flags, size, alignment, initializer values,
  initializer symbol information, and address materialization policy.
- `bir::LoadGlobalInst` and `bir::StoreGlobalInst` carry an access global name,
  `LinkNameId`, byte offset, alignment, and optional `MemoryAddress`.
- `MemoryAddress` / `MemoryAccessProvenance` can record base identity,
  requested byte range, layout authority, dynamic-array facts, and range
  verdicts for an access after lowering has produced them.
- `BirFunctionLowerer::GlobalAddress` and the `resolve_global_gep_address` /
  `resolve_relative_global_gep_address` helpers can compute global byte offsets
  from a LIR GEP in lowering-local context.
- `resolve_global_dynamic_scalar_array_access` and related helpers can describe
  some dynamic global array accesses while lowering.
- `PreparedMemoryAccess` and `PreparedGlobalObjectData` publish prepared
  memory and selected object-data facts, but they are not GEP-path certificates.

## Missing Lower Prerequisite

The exact lower prerequisite is a production BIR semantic
`global_static_gep_authority` certificate surface, populated during or
immediately after semantic LIR-to-BIR lowering, before final admission.

That lower certificate must publish one record per candidate GEP and must be
available only when these identities match:

- global/static source object identity: global name, `LinkNameId`, storage
  kind, value type/type text, size, alignment, thread-local/constant flags, and
  selected object-data identity if object data is required;
- derivation/provenance identity: result pointer value, base global/static
  object, base byte offset, relative-base flag when applicable, and whether the
  path was direct global, relative global pointer, or global pointer alias;
- layout path authority: aggregate/array projection sequence, element type,
  element size, element count, normalized byte offset, total object size, and
  layout-authority kind;
- dynamic index/range authority: the dynamic index `bir::Value`, proven element
  count/stride/base offset, in-bounds verdict, and any required dominance or
  non-clobber coordinate if the index is not constant;
- coordinate identity: producer function, block label, instruction index,
  lookup key, operation role, and coordinate status for the LIR GEP that
  produced the pointer/address;
- consumer linkage when applicable: the load/store/call/intrinsic consumer that
  uses the derived address must match the authority record and may not be
  inferred from final homes or target lowering.

## Available Contract

Final semantic global/static GEP admission may report `Available` only from a
matching available `global_static_gep_authority` record. It must not become
available from:

- `bir::Global` alone;
- `LoadGlobalInst` or `StoreGlobalInst` byte offsets alone;
- transient `GlobalAddress` or `GlobalPointerMap` entries without a published
  record;
- prepared object data, relocations, object labels, or final homes alone;
- raw testcase/source shape, names, labels, dump order, target behavior, or
  object emission.

## Unavailable Statuses

The lower prerequisite and final admission should keep these outcomes
distinguishable:

- `MissingGlobalSourceObject`
- `MissingGlobalLinkIdentity`
- `GlobalSourceUnsupported`
- `MissingSelectedObjectData`
- `MissingGlobalLayout`
- `MissingLayoutPath`
- `MissingDerivation`
- `MissingDerivedPointerIdentity`
- `MissingDynamicIndexIdentity`
- `MissingDynamicRangeAuthority`
- `RangeProofNotDominatingConsumer`
- `RangeProofPathNotCoveringConsumer`
- `IndexValueClobberedBeforeConsumer`
- `ElementOutOfBounds`
- `ElementNotScalar`
- `LayoutRangeMismatch`
- `PointerOrFormalProvenanceBoundary`
- `StringOrGlobalPointerProvenanceBoundary`
- `RuntimeOrStringIntrinsicBoundary`
- `AggregateOrMemberBoundary`
- `RawShapeOnly`
- `TargetOnlyOrFinalHomeOnly`
- `PreparedBirCoordinateConfusion`

`status_matrix.tsv` maps these statuses to the Step 1 rows and routing
decisions.

## Row-Specific Contract

- `src/20000717-4.c`: admit only an available authority for global `s` with the
  nested projection `slot[0].field[!toggle]`, a proven boolean dynamic index,
  and an in-bounds scalar `int` byte range.
- `src/20031214-1.c`: admit only an available authority for global `g` field
  array `n[j]`, with a proven `j` range over the two elements.
- `src/20080424-1.c`: admit only when the authority preserves global `g`
  through the inlined `foo(g)` parameter and proves `x[k]` / `x[k - 8]` within
  the multidimensional array object.
- `src/20120808-1.c`: admit only when the authority ties `p = d + i` and the
  looped `++p` use back to global byte array `d[32]`, with explicit volatile
  index/range handling and byte-element bounds.
- `src/20051104-1.c`: fail closed as
  `StringOrGlobalPointerProvenanceBoundary` until a separate
  string/global-pointer provenance certificate identifies the pointed object
  behind `s.name[s.len]`.
- `src/ieee/copysign2.c`: fail closed as `RuntimeOrStringIntrinsicBoundary`
  until runtime/string intrinsic consumer authority covers the copysign/memcmp
  harness use.

## Disposition

Current producer surfaces are insufficient for final semantic global/static GEP
admission. Step 3 should route to the lower prerequisite packet:

`Publish Global/Static GEP Authority Certificates`

That packet should add the durable `global_static_gep_authority` producer
surface from existing LIR-to-BIR global address/layout/range/coordinate inputs.
Only after that lower certificate exists should final semantic global/static
GEP admission consume it.
