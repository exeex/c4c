# Structured Opaque Pointer Byte-Range Provenance

## Goal

Replace the `allow_opaque_ptr_base && stored_type == I8` compatibility bridge
with structured pointer-provenance facts for prepared memory accesses.

## Why This Exists

The post-286 cleanup decided that the current opaque pointer byte-offset rule
is a compatibility bridge, not a durable semantic carrier. Prepared
`memory_accesses` currently publish the selected address as
`PreparedAddressBaseKind::PointerValue` plus offset, size, alignment, and
`can_use_base_plus_offset`; those facts do not prove that the access is inside
a known source object range.

Further prepared `memory_accesses` demotion or narrowing needs explicit base
identity, object extent, requested byte range, layout authority, and dynamic
array range facts before the compatibility rule can safely be replaced.

## In Scope

- Add a structured provenance/range carrier before
  `can_address_scalar_subobject`.
- Track stable base identity for pointer SSA/value names, local slots, globals,
  formal/byval/sret identities, and explicitly unknown runtime bases.
- Track known object extent, valid byte-offset bounds, completeness, requested
  access offset/size/alignment/address space/volatility, and scalar type.
- Use structured layout or scalar layout facts as range authority when
  available.
- Preserve dynamic-array facts such as element count, stride, dynamic index,
  and bounded range when provable.
- Publish prepared `memory_accesses` records that distinguish syntactic
  base+offset usability from proven in-range object provenance.
- Keep existing opaque byte-addressed behavior until each route can prove the
  access range or fail closed explicitly.

## Out of Scope

- Removing the compatibility rule before all required routes publish structured
  range facts or explicit unknown/opaque verdicts.
- Treating `can_use_base_plus_offset` as object-range proof.
- Target-specific prepared/prealloc acceptance that bypasses target-independent
  provenance facts from BIR lowering.
- Rewriting unrelated prepared publication surfaces.

## Owned Files

- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- pointer-address producers in `src/backend/bir/lir_to_bir/memory/`,
  `src/backend/bir/lir_to_bir/module.cpp`, and relevant call-return pointer
  carriers
- prepared publication in `src/backend/prealloc/addressing.hpp` and lookup /
  publication-plan consumers that need to distinguish syntactic base+offset
  use from proven in-range provenance
- focused BIR/prepared tests for opaque pointer byte offsets, unknown extents,
  dynamic arrays, stale prepared rows, and fail-closed out-of-range access

## Acceptance Criteria

- Opaque pointer byte-offset admission has a structured carrier for base
  identity, object extent when known, requested byte range, layout authority,
  and dynamic-array range facts.
- Prepared `memory_accesses` can report whether base+offset is only
  syntactically usable or is proven within a known object range.
- Missing identity, unknown extent, negative or overflowing offset,
  access-size overflow, stale prepared rows, and unproven dynamic ranges fail
  closed or remain explicit unknown/opaque compatibility cases.
- Focused proof includes build proof, `backend_lir_to_bir_notes`,
  `backend_prepared_lookup_helper_test`,
  `backend_riscv_prepared_edge_publication_test`, and focused prepared
  memory-access subsets consuming `PreparedAddressBaseKind::PointerValue` and
  `can_use_base_plus_offset`.

## Reviewer Reject Signals

- The patch removes the compatibility rule before every opaque pointer route
  has a structured range fact or explicit fail-closed verdict.
- The route treats `can_use_base_plus_offset` as proof of object-range
  validity.
- Arbitrary `i8`-based offsets are accepted without checking access size,
  overflow, and known base extent when that extent is available.
- Rendered type text is used as the primary range authority when a structured
  layout id or explicit base extent exists.
- Stale-row, duplicate-row, or cross-block prepared `memory_accesses`
  rejection is weakened to keep a narrow testcase green.
- Target-specific acceptance is added in prepared/prealloc instead of
  publishing target-independent provenance facts from BIR lowering.
