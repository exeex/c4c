# 499 Step 2 Semantic GEP Local-Memory Contract

Step 2 defines the narrow BIR semantic GEP local-memory admission contract for
the direct local-object candidates identified by Step 1.

## Inputs

- Step 1 classification:
  `build/agent_state/499_step1_gep_local_memory_classification/summary.md`
- Direct local candidates:
  `src/pr80421.c`, `src/930614-2.c`, `src/pr24851.c`
- Existing authority surfaces in `src/backend/bir/bir.hpp`:
  `LocalArraySourceObjectRecord`, `LocalArrayAddressDerivationRecord`,
  `LocalArrayElementPathRecord`, `LocalArrayIndexRangeCheckerInputRecord`, and
  `LocalArrayLocalAddressProvenanceRecord`.

## Available Status

The semantic GEP admission record should use `LocalArrayCarrierStatus` as its
status vocabulary.

`Available` is allowed only when all of these inputs match the same local-object
GEP identity:

- `LocalArrayLocalAddressProvenanceRecord::status == Available`
- `LocalArrayLocalAddressProvenanceRecord::checker_status == Available`
- `LocalArrayLocalAddressProvenanceRecord::source_object`,
  `derivation`, `element_path`, and `checker_input` are non-null and point to
  matching records.
- `LocalArrayElementPathRecord::status` is `Available` or the pre-checker
  `MissingIndexRangeProof` state consumed by the matching checker input.
- `LocalArrayElementPathRecord::lir_producer_operation_role ==
  AddressDerivation`.
- `LocalArrayElementPathRecord::lir_producer_coordinate_status == Available`.
- `LocalArrayAddressDerivationRecord::kind` is not `Unknown`.
- Exactly one dynamic local-array index is identified when the path is dynamic;
  constant-only local-object GEPs may be represented with no dynamic proof
  requirement only if the implementation records that distinction explicitly.
- `element_type != Void`, `element_size_bytes > 0`, `scalar_in_bounds == true`,
  and `byte_offset + element_size_bytes <= source_total_size_bytes` when the
  source total size is known.

## Required Fields

The Step 3 record should preserve enough information for semantic admission and
later consumers without re-deriving lower facts:

| Authority | Required fields |
| --- | --- |
| Source object | `source_object_name`, `element_type`, `type_text`, `element_count`, `element_size_bytes`, `total_size_bytes`, `align_bytes`, `element_slots`, `status` |
| Derivation/provenance | `derived_pointer_name`, `derivation_kind`, `LocalArrayAddressDerivationRecord::result_name`, `source_object_name`, `base_view_name`, `base_index`, `status` |
| Element path | `element_result_name`, `source_object_name`, `derivation_result_name`, `indices`, `element_type`, `element_size_bytes`, `byte_offset`, `element_count`, `scalar_in_bounds`, `status` |
| Layout/range | `checker_status`, `LocalArrayIndexRangeCheckerInputRecord::status`, `checker_record.status`, normalized bounds from the checker/proof fact when dynamic |
| Coordinate | `lir_producer_lookup_key`, `lir_producer_function_name`, `lir_producer_block_label`, `lir_producer_instruction_index`, `lir_producer_coordinate_status`, `lir_producer_operation_role` |

The implementation must copy these fields from matching production authority
records. It must not reconstruct them from raw LIR/BIR shape, value names,
final homes, RV64 object behavior, testcase names, or target lowering state.

## Unavailable Statuses

Step 3 should preserve these fail-closed statuses as distinct outcomes:

| Condition | Status |
| --- | --- |
| No element path for the candidate | `MissingElementPath` |
| No matching source object | `MissingSourceObject` |
| Source object is not local or is global/static | `SourceObjectNotLocal` or `GlobalSourceObject` |
| Missing layout/object-to-slot relation | `MissingSourceObjectLayout` or `MissingObjectToSlotRelation` |
| No matching derivation | `MissingDerivation` |
| Derivation exists but is not proven local | `DerivationNotProvenLocal` |
| Missing derived pointer identity | `MissingDerivedPointerIdentity` |
| No stable dynamic index identity for a dynamic GEP | `MissingIndexIdentity` |
| Missing or unavailable checker input/range proof | `MissingIndexRangeProof` with the underlying `checker_status` preserved |
| Proof does not dominate/cover the consumer | `RangeProofNotDominatingConsumer` or `RangeProofPathNotCoveringConsumer` |
| Index clobbered before the consumer | `IndexValueClobberedBeforeConsumer` |
| Out-of-bounds element or byte range | `ElementOutOfBounds` |
| Non-scalar element for this focused contract | `ElementNotScalar` |
| Layout/range mismatch | `LayoutRangeMismatch` |
| Unknown, pointer-derived, or integer-pointer provenance | `UnknownProvenance` or `IntegerPointerRoundTrip` |
| Global/static object GEP row | `GlobalSourceObject` |
| Aggregate/member/flexible/alias boundary | `AggregateOrMemberBoundary` |
| Union/object-representation boundary | `UnionOrObjectRepresentationBoundary` |
| Variadic/byval/va_arg row | `VariadicOrVaArgBoundary` |
| Runtime/call/string intrinsic row | `RuntimeOrCallBoundary` |
| F128/complex/vector/volatile/atomic row | `F128ComplexVectorOrVolatileAtomicBoundary` |
| Bootstrap row | `BootstrapBoundary` |
| Raw-shape-only evidence | `RawShapeOnly` |
| Target/final-home-only evidence | `TargetOnlyOrFinalHomeOnly` |
| Coordinate mismatch, duplicate, or cross-record identity confusion | `PreparedBirCoordinateConfusion` |

## Candidate-Specific Contract

- `src/pr80421.c`: admit only the local stack array path rooted in `char c[]`
  when the derivation for `f = c + 390` and the dynamic `f[g]` element path are
  backed by matching available provenance and checker input.
- `src/930614-2.c`: admit only the local multidimensional array path
  `x[i][k][j][l]` when every dynamic index used by the element path has explicit
  range/checker authority or is represented as a constant index in the element
  path.
- `src/pr24851.c`: admit the local array negative subscript shape only when the
  normalized path remains in-bounds for the original local object and the
  derivation/proof chain describes that relationship explicitly.

## Step 3 Disposition

Step 3 can implement the local-object GEP producer. No lower prerequisite is
currently blocking the contract, because the existing local-array authority
chain already exposes source object, derivation, element path, checker/range,
and coordinate fields. Step 3 must fail closed whenever those records are absent
or non-available.

Exact Step 3 packet: implement a production BIR semantic GEP local-memory
admission record populated only from matching available
`local_array_local_address_provenances` for local-object
`AddressDerivation` paths, with focused tests for the three local candidate
shapes and the boundary statuses above.
