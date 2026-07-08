# 05. Follow-Up Recommendations

Status: Step 6 complete

## Purpose

This file records the implementation handoff from the pointer/address semantic
model research. It opens only narrow follow-up ideas where the first owner and
proof surface are clear enough to be useful. Families whose implementation
owner depends on later `PreparedMirView` design or target-consumption
sequencing remain deferred instead of being folded into this route.

## Recommendation Summary

| Recommendation | First owner | Prerequisites | Proof surface | Reviewer reject signals | Status |
| --- | --- | --- | --- | --- | --- |
| `ideas/open/599_pointer_base_plus_offset_selected_authority.md` | Shared prepared/prealloc pointer-arithmetic authority | Step 3 authority classification; Step 4 fail-closed rules; closed idea 587 selected freshness vocabulary | Audit pointer-base-plus-offset consumers, define selected base/result/delta/use/program-point authority, and migrate at most one representative consumer with stale, wrong-value, wrong-delta, wrong-use, range-only, and target-shape-only rejection proof | Reject home-shape, byte-delta, stack/register placement, or target-offset encodability as authority without selected base freshness; reject broad target migration or testcase-shaped offsets | Opened |
| `ideas/open/600_pointer_value_memory_use_freshness_authority.md` | Shared prepared/prealloc pointer-value memory-use authority | Step 3 authority classification; Step 4 fail-closed rules; closed ideas 587 and 589 freshness/source ownership boundaries | Audit pointer-value memory producers and one representative load/store consumer, define selected pointer-value freshness for the exact memory use, and prove range/layout/target-shape-only evidence fails closed | Reject treating `prepared_pointer_value_memory_has_proven_authority(...)`, object extent, offset range, or target memory operands as pointer freshness; reject broad loaded/store-source or target migration | Opened |

These two ideas are narrow enough to stand alone because the research located
a first owning layer and a dimension-based proof shape. They deliberately do
not claim target-wide migration or global pointer/address closure.

## Deferred Families

| Family | Why deferred | Missing first owner or proof surface |
| --- | --- | --- |
| Local-array and global static semantic GEP target consumption | The semantic authority is decided: `Available` local-array and global static semantic GEP records authorize their selected address derivations. The remaining question is how MIR or target consumers should read that authority instead of inferring from memory access, relocation, local layout, or target operands. | First implementation owner depends on idea 591's `PreparedMirView` contract shape and phased target migration plan. Proof surface must show one view or target consumer reads semantic GEP authority and keeps non-`Available`, relocation-only, range-only, and target-shape-only cases rejected. |
| Loaded-value and store-source freshness for global or pointer-value memory | The research classified exact global symbol memory access as address/range authority and pointer-value memory range proof as support, but it did not define loaded scalar freshness or store-source freshness ownership. | Missing first owner for the loaded/store value freshness route and missing proof surface that separates value freshness from address legality. |
| Aggregate-adjacent branch, select, call, publication, and non-branch pointer/address consumers | The closed branch pointer stack-source queue proves only RV64 fused pointer branch stack-slot `Lhs`/`Rhs` selected freshness at the exact branch terminator use. Other use families need their own contracts. | Ownership and proof surface vary by use kind. Existing open `ideas/open/598_select_carrier_alias_freshness_contract.md` covers one select-adjacent freshness question; other families should split only when a first owner and representative consumer are known. |
| Relocation/materialization-only and target-local operand-shape routes | Relocation/materialization and final operand shape are target-consume facts. This research found no need for a new implementation idea solely to restate that they are not semantic authority. | No semantic implementation owner is needed unless a later consumer audit finds a concrete route accepting relocation-only or target-shape-only evidence as authority. |

## Recommendation Details

### Pointer Base Plus Offset Selected Authority

The first follow-up should define selected authority for
`PreparedValueHomeKind::PointerBasePlusOffset` and
`PreparedPointerBasePlusOffsetFact`. The authority must name:

- base pointer freshness
- result pointer identity
- byte delta
- consuming use kind
- program point or equivalent use coordinate

The first owner is the shared prepared/prealloc pointer-arithmetic authority
layer because the fact is published before targets consume or reject it.
Targets may later consume the selected result, but target encodability is not
the authority.

The proof should vary base value, result value, delta, use kind, and program
point. A reviewer should reject any slice whose main proof is that one target
can encode one offset, one testcase now lowers, or one unsupported diagnostic
changed.

### Pointer-Value Memory-Use Freshness Authority

The second follow-up should define selected freshness for pointer-value
indirect memory uses. `PreparedAddressBaseKind::PointerValue` access records
and `prepared_pointer_value_memory_has_proven_authority(...)`-style checks
prove important address legality dimensions, but they do not prove the named
pointer value is fresh at the load/store use.

The first owner is the shared prepared/prealloc pointer-value memory-use
authority layer because it must sit above target memory operand formation. The
proof should vary pointer value name, memory instruction use, offset/range,
provenance base, layout authority, and target operand shape.

A reviewer should reject any slice that treats range, extent, local layout,
or target operand shape as selected pointer freshness. Loaded-value freshness,
store-source freshness, and broad target migration should remain separate.

## Impact On Idea 591

Idea 591 should consume this research before designing `PreparedMirView`
pointer/address fields. The view may expose:

- selected branch stack-source freshness for the already closed RV64 pointer
  branch `Lhs`/`Rhs` subset
- semantic GEP availability records as address-derivation authority, with
  target consumption still separate
- prepared memory access and address materialization records as support or
  target-consume facts according to their fact class
- unavailable or deferred statuses for pointer-base-plus-offset,
  pointer-value memory-use freshness, and unresolved loaded/store-source
  freshness

Idea 591 must not turn deferred families into a generic pointer validity bit
or infer authority from view completeness. If the future view needs semantic
GEP target consumption, it should first decide the view shape and then open a
narrow consumer migration with non-`Available`, relocation-only, range-only,
and target-shape-only rejection proof.

## Step 6 Conclusion

The implementation handoff is intentionally small:

1. Open pointer-base-plus-offset selected authority.
2. Open pointer-value memory-use freshness authority.
3. Defer semantic GEP target consumption and loaded/store-source freshness
   until the first owner and proof surface are settled by idea 591 or a later
   consumer audit.

This preserves the research boundary: no implementation files, tests,
expectations, unsupported markers, allowlists, runtime behavior, or harness
behavior are changed by this package.
