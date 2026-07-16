# 837 Step 1 Evidence Baseline

Evidence revision: `ac4af813dd27f29a1bbe48aeb28e67ed1eebec11`

This is an evidence/classification baseline only.  It neither accepts a type
family implementation nor changes the parked 836 route.

## Scope inspected

- The exact closed records named by 837: 759, 760, 761, 763, 775, 776, 754,
  798, 801, 803, 811, 814, 815, 762, and 832--835; the open contracts 734,
  795, 797, 812, 813, 829--831, and parked 836; and the historical,
  non-owner `origin/new_bir` Idea 746.
- `src/codegen/lir/types.hpp` (lines 62--276): one `LirTypeRef` owns rendered
  text, inferred `LirTypeKind`, builtin width facts, named-composite IDs,
  array children, anonymous fields, mutable `str()`, implicit string
  conversions, equality, and `render_llvm()`.
- `src/codegen/lir/ir.hpp` (the `LirTypeRef` fields reported throughout lines
  118--1107): operations additionally carry parallel aggregate result,
  vector lane/element/mask, signature, declaration, and owner mirrors.
- HIR-to-LIR lowering: `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:90-112`,
  `:167-180`, `:349-377`, and `:1797-1823`; call/lvalue routes under
  `src/codegen/lir/hir_to_lir/`; and `src/codegen/shared/llvm_helpers.hpp`
  at `483-698` and `741-767`.
- `src/codegen/lir/verify.cpp:195-240` and its operation-family callers:
  `require_module_type_ref` recursively checks anonymous fields, while known
  named structs still validate a `StructNameId` mirror against module lookup.
  Printer, call-compatibility, reference-collector, and LIR-to-BIR consumers
  were located through `src/codegen/lir/lir_printer.cpp`, `call_args*`, and
  `src/backend/{bir,legacy}/lir_to_bir*`.

## Current model facts

`LirTypeRef` is therefore a universal representation, not merely a string
wrapper: it blends scalar classification, nominal named-composite identity,
recursive array/anonymous-layout shape, function/call text, compatibility
storage, rendering, and equality.  `render_llvm()` is already one-way for the
array and anonymous-aggregate islands, but remaining construction and many
verification paths still read/classify `text_` (for example
`verify.cpp:128-166`).  This preserves accepted bounded islands without
proving a coherent family model.

The historical BIR 746 record is only a design reference: enum tags,
`constexpr` schema/traits, explicit switches, checked handling, and stable
arena refs are useful constraints; it is neither an active LIR owner nor a
dependency that can satisfy a 837 row.

## HIR occurrence-to-canonical-definition seam

HIR already has canonical aggregate definitions and a module owner index:
`Module::struct_defs`, `struct_def_order`, and
`find_struct_def_tag_by_owner` are consumed by
`find_typespec_aggregate_layout` (`llvm_helpers.hpp:741-767`).
`HirStructDef` contains the field/layout facts used by lowering.

What is missing is a stable occurrence-to-definition carrier.  A
`QualType`/`TypeSpec` aggregate occurrence is converted by
`lir_owned_type_spec` (`hir_to_lir.cpp:90-112`) by clearing parser pointers
and reconstructing a `HirRecordOwnerKey`; it then looks up a tag in the
module.  The cross-table helpers at `llvm_helpers.hpp:641-698` may again
canonicalize through tag spelling.  A future aggregate family must instead
make every aggregate-bearing `QualType` resolve through a stable reference to
one canonical module-owned `HirStructDef` (an assessed `HirAggregateId` or
`HirAggregateRef`, or a proven equivalent), never a reconstructed key,
rendered tag, or parser pointer.  This is a seam to decide in Step 2, not a
claim that a carrier exists today.

## Residual 836 evidence remains three contracts

| Group | Historical cases | Current seam | Status/preservation |
| --- | --- | --- | --- |
| incomplete structured owner key | 50, 52, 510, 661, 688, 692, 3038 | `lir_owned_type_spec` throws before lookup when `typespec_aggregate_owner_key(hir_type, mod)` is absent | unresolved; preserve fail-closed behavior for incomplete/foreign identity, do not add a tag fallback |
| present but unmatched module owner | 201, 202, 217, 1529 | the key exists but `find_struct_def_tag_by_owner` returns no nonempty owner | unresolved; preserve module ownership rejection and do not treat text equality as ownership |
| legitimate no-owner rendered compatibility | 46 | compatibility routes such as `frontend/hir/impl/expr/builtin.cpp:123-126` deliberately consult text only after structured-owner misses are fenced | unresolved compatibility contract; preserve its explicit separation from structured misses rather than weakening diagnostics |

The parked evidence is the rejected 3026/3038 candidate against the accepted
3038/3038 baseline: twelve failures, ten newly classified failures, and a
decreased pass count.  It is historical input, not a fresh reproduction.

## Accepted capability and bounded exclusions

Accepted work retains enum-first builtin identity (759), constructor inventory
and explicitly named runtime-text boundaries (760), structured call/signature
mirror precedence and raw/inline-asm boundaries (761), and selected
builtin/named-composite/array islands (763).  It also retains selected
aggregate/vector operation identity (754), aggregate operand provenance (798),
anonymous layout facts (801), selected aggregate SSA producers (803), vector
carrier and splat/mask coherence (811/814/815), module declaration/type-shadow
precedence (762), PHI/expression carrier work (775/776), and bounded owner or
parameter repairs (832--835 and 833).

These are bounded capabilities.  They exclude generic recursive type closure,
universal aggregate ownership, every ABI/body parameter form, full type-tree
lowering, global initializer semantics, generic vector/function families, and
deletion of `runtime_text`, mutable string access, or `LirTypeRef`.

## Open dependencies and preserved return state

- 734 receives only an already accepted typed LIR handoff; 797 is terminal
  disposition/dispatcher/proof convergence, not producer repair.
- 795 is bounded; the current body-parameter chain remains
  829 -> 830 -> 831 -> 836.  Call signature facts and body-use identity remain
  separate first owners.
- 812 must refresh its string inventory after the type-family queue has
  accepted capabilities; 813 then routes residual non-type string work.
- 836 is parked at unchanged Step 1.  After 837 and its priority successors,
  it resumes only on fresh evidence of an independently owned residual family.
  After a bounded 836 acceptance and proof, reactivate 831 at unchanged Step
  4 for the comparable full-suite gate; never return directly to 830 or 829.

