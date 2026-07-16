# Nominal Family Boundary Decisions

Evidence revision: `da06fe48f1a8b534d29be14357b865035082cdf3`

These are architecture decisions for later first-owner scopes, not code or a
claim that any migration is complete.

## Chosen boundaries

| Family | Decision | Inclusion and exclusion |
| --- | --- | --- |
| Scalar | A compact nominal `LirScalarRef`/store owns builtin identity, widths, pointer/void ABI leaves where separately admitted, and scalar operation typing. | Scalar-only schemas accept only `LirScalarRef`; vectors, aggregates, and signatures are compile-time invalid. Opaque semantics remain evidence-needed rather than silently scalarized. |
| Vector | A nominal `LirVectorRef`/store owns exactly lane count and an element family ref. | Row-local `LirNativeVectorShape`, mask type strings, and element text become derived adapters during migration, never competing semantic authority. |
| Aggregate | A canonical module-owned `LirAggregateRef`/store owns identity, struct/union kind, fields, layout, projections, and owner coherence. | Named and anonymous aggregates are store entries; field and nested aggregate children are typed refs. No name, tag, rendered text, parser pointer, or operation-local layout is identity. |
| Function signature | A nominal `LirFunctionSignatureRef` owns return ref, ordered parameter refs, ABI/signature flags, and declaration/call composition. | It composes only explicitly admitted value-type alternatives and preserves raw call/extern text solely behind named one-way adapters. It does not absorb 829/830 body-use identity. |
| First-class values | Use a small boundary-specific tagged union only at evidenced call argument/result, PHI, select, and return boundaries. | Each union has an explicit enum kind and exact allowed alternatives; no unrestricted `variant`, universal ID, optional bag, RTTI hierarchy, or implicit cross-family conversion. |

## Chosen HIR occurrence carrier and aggregate graph

Choose `HirAggregateId` with `HirAggregateRef { ModuleId, HirAggregateId }`
(or an existing module identity type proved bit-for-bit equivalent) as the
required HIR occurrence-to-definition carrier.  The ID is allocated by the
canonical HIR module aggregate store alongside `HirStructDef`; it is stable for
that module and survives parser-storage teardown.  Every aggregate-bearing
`QualType` holds this ref after HIR materialization.

This replaces the current path in `lir_owned_type_spec` that clears
`record_def` then rebuilds `HirRecordOwnerKey` and a tag lookup.  The selected
contract deliberately forbids reconstructing identity from tags, rendered
text, cross-table spelling, or parser pointers.  Lowering checks the module
component, resolves the canonical definition once, and interns/maps the
`HirAggregateRef` to one `LirAggregateRef` for the LIR module.  Unknown,
incomplete, foreign, stale, and wrong-module refs fail closed.

The aggregate store graph is recursive and typed:

```
QualType aggregate occurrence -> HirAggregateRef -> canonical HirStructDef
                                      | lowering map (module scoped)
                                      v
                               LirAggregateRef -> LIR aggregate entry
                                                     -> ordered FieldTypeRef children
                                                        (scalar | vector | aggregate | allowed value form)
```

Fields, projections, layout and nested aggregates are retained in the entry;
no lowering route may flatten hierarchy into text.  Anonymous aggregates get a
canonical entry too; local/template/typedef/alias occurrences must resolve to
their canonical definition or an explicit no-owner compatibility boundary, not
an inferred tag.  The three 836 groups stay distinct: absent/incomplete ref,
present ref without matching module owner, and explicitly legitimate no-owner
compatibility.

## Access, separation, and consumer rules

- Each family and value union has an enum kind plus C++20 traits and custom
  checked `isa`/`cast`/`dyn_cast`-style access.  Access is exhaustive and
  rejects wrong family/kind; it uses neither `std::dynamic_cast`, RTTI,
  vtables, TableGen, nor an external schema DSL.
- Schema fields use nominal refs so a scalar-only operation cannot receive a
  vector, aggregate, or signature at compile time.  A conversion exists only
  at an explicitly declared first-class value boundary.
- Verifier, semantic dispatch, and LLVM printer receive overloads by family.
  Aggregate overloads check store/module identity and recurse through typed
  children; vector overloads check lane/element coherence; signature overloads
  check ordered allowed parameters.  Foreign, stale, wrong-module, malformed,
  and wrong-family inputs fail closed.
- Rendering is one-way.  Printer overloads derive LLVM text from family facts;
  compatibility mirrors may be checked against those facts but never parsed to
  repair identity, shape, ownership, or dispatch.
- Adapters are explicit, one-way, consumer-named, and temporary.  Their final
  deletion gates are the matrix gates: no named consumer remains, parity and
  malformed/foreign behavior are proved, and no recursive child is text-primary.

## Dependency implications

The canonical aggregate store/ref contract is the first shared prerequisite,
then function-signature composition where aggregate/value facts require it.
Vector/scalar, restricted union, direct HIR construction, and family-overloaded
consumers follow only by their evidence-backed first owner.  Universal
`LirTypeRef`, semantic `runtime_text`, mutable `str()`, implicit string
conversion, textual equality/classification, and expired adapters are terminal
deletion work feeding 797.  Only after accepted type-family capabilities may
812 refresh its inventory and 813 route residual non-type text.

