# Structured Opaque Pointer Byte-Range Provenance Runbook

Status: Active
Source Idea: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md

## Purpose

Replace the opaque pointer `allow_opaque_ptr_base && stored_type == I8`
compatibility bridge with structured byte-range provenance facts for prepared
memory accesses.

## Goal

Publish prepared memory-access records that can distinguish syntactic
base-plus-offset usability from proven in-range object provenance, while
keeping existing opaque byte-addressed behavior until each route has structured
range proof or an explicit unknown/fail-closed verdict.

## Core Rule

Do not treat `can_use_base_plus_offset` as object-range proof. Any replacement
for the compatibility bridge must carry base identity, object extent or
unknown-extent verdict, requested byte range, layout authority, and dynamic
array range facts when they are available.

## Read First

- `ideas/open/289_structured_opaque_pointer_byte_range_provenance.md`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- Pointer-address producers in `src/backend/bir/lir_to_bir/memory/`
- Relevant pointer carriers in `src/backend/bir/lir_to_bir/module.cpp` and
  call-return paths
- Prepared publication and consumers in
  `src/backend/prealloc/addressing.hpp` and prepared lookup / publication-plan
  tests

## Current Targets

- The prepared `memory_accesses` address/provenance carrier before
  `can_address_scalar_subobject`.
- Stable base identity for pointer SSA/value names, local slots, globals,
  formal/byval/sret identities, and explicitly unknown runtime bases.
- Known object extent, valid byte-offset bounds, completeness, requested
  access offset/size/alignment/address space/volatility, and scalar type.
- Structured layout or scalar layout facts used as range authority.
- Dynamic array element count, stride, dynamic index, and bounded range facts.
- Prepared lookup and publication-plan consumers that must distinguish
  syntactic base-plus-offset use from proven in-range provenance.

## Non-Goals

- Do not remove the compatibility rule before all required opaque pointer
  routes publish structured range facts or explicit unknown/fail-closed
  verdicts.
- Do not use `can_use_base_plus_offset` as a proxy for known object extent or
  range validity.
- Do not add target-specific prepared/prealloc acceptance that bypasses
  target-independent provenance facts from BIR lowering.
- Do not rewrite unrelated prepared publication surfaces.
- Do not claim progress through expectation-only changes, helper renames, or
  rendered type text when structured layout or explicit extent facts exist.

## Working Model

Prepared memory-access lowering needs two separate concepts:

- syntactic address usability: whether an address can be printed or emitted as
  base plus byte offset
- provenance range proof: whether the requested byte range is known to be
  inside a source object or must remain unknown/fail closed

The structured carrier should be created before scalar-subobject acceptance and
should preserve compatibility only as an explicit verdict. Known extent and
layout authority should prove positive in-range cases. Missing identity,
unknown extent, negative or overflowing offset, access-size overflow, stale
prepared rows, and unproven dynamic ranges must not silently become proven
accesses.

## Execution Rules

- Keep changes narrow and packetized around the memory-access provenance path.
- Prefer structured enums/records over inferred meaning from booleans or
  rendered type strings.
- Preserve existing supported opaque byte-addressed behavior until the matching
  route has structured proof or an explicit compatibility/unknown verdict.
- Add focused tests only to prove provenance carrier behavior, range checks,
  dynamic-array facts, stale-row rejection, and fail-closed edges; do not
  weaken supported-path expectations.
- Every code-changing step needs fresh build proof plus the
  supervisor-selected focused test subset.
- Broader validation should include `backend_lir_to_bir_notes`,
  `backend_prepared_lookup_helper_test`,
  `backend_riscv_prepared_edge_publication_test`, and focused prepared
  memory-access subsets consuming `PreparedAddressBaseKind::PointerValue` and
  `can_use_base_plus_offset`.

## Step 1: Inventory Current Opaque Pointer Provenance Routes

Goal: establish the exact current compatibility bridge, address facts, and
prepared consumers before adding a structured carrier.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- pointer-address producers in `src/backend/bir/lir_to_bir/memory/`
- prepared/prealloc address consumers

Actions:

- Locate the `allow_opaque_ptr_base && stored_type == I8` admission rule and
  every path that relies on it.
- List the facts currently published for prepared `memory_accesses`, including
  selected address base kind, offset, size, alignment,
  `can_use_base_plus_offset`, address space, volatility, and scalar type.
- Identify where base identity, object extent, layout authority, and dynamic
  array range facts already exist or are missing.
- Identify focused tests that cover opaque pointer byte offsets, unknown
  extents, stale prepared rows, dynamic arrays, and fail-closed out-of-range
  behavior.

Completion check:

- `todo.md` records the route inventory, current compatibility semantics,
  missing structured facts, and initial focused proof subset for later steps.

## Step 2: Define the Provenance and Byte-Range Carrier

Goal: introduce a structured carrier that can represent known in-range,
unknown/opaque compatibility, and fail-closed access verdicts without changing
observable behavior.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`

Actions:

- Define stable base identity cases for pointer SSA/value names, local slots,
  globals, formal/byval/sret identities, and unknown runtime bases.
- Define object-extent and requested-range facts, including completeness,
  offset, size, alignment, address space, volatility, scalar type, and overflow
  status.
- Define layout authority fields for structured layout, scalar layout, and
  explicit unknown authority.
- Define dynamic-array facts for element count, stride, dynamic index, and
  bounded range when provable.
- Keep existing boolean address usability fields available for consumers while
  adding the separate range-provenance verdict.

Completion check:

- The carrier compiles and can be populated with neutral/unknown verdicts
  without broadening or narrowing prepared memory-access behavior.

## Step 3: Populate Base Identity and Known Extent Facts

Goal: make pointer-address producers publish structured identity and extent
facts where the lowering pipeline already has the information.

Primary targets:

- pointer-address producers in `src/backend/bir/lir_to_bir/memory/`
- `src/backend/bir/lir_to_bir/module.cpp`
- relevant call-return pointer carriers

Actions:

- Populate identity for pointer SSA/value names, local slots, globals, and
  formal/byval/sret bases when available.
- Populate known object extent and completeness when layout or scalar facts are
  authoritative.
- Preserve explicit unknown runtime-base verdicts where identity or extent is
  not provable.
- Ensure negative offsets, offset overflow, and access-size overflow cannot be
  represented as proven in-range facts.

Completion check:

- Focused build/test proof shows existing opaque pointer behavior is preserved,
  while the structured carrier records known identity/extent or explicit
  unknown verdicts for the inventoried routes.

## Step 4: Prove Requested Range and Dynamic Array Bounds

Goal: use the structured carrier to prove requested byte ranges only when the
source object or dynamic array facts justify them.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- dynamic-array lowering paths that feed prepared `memory_accesses`

Actions:

- Compute requested byte range from offset and access size with overflow
  checks.
- Use structured layout or scalar layout facts as range authority when
  available.
- Preserve dynamic-array element count, stride, dynamic index, and bounded
  range facts when provable.
- Fail closed or remain explicitly unknown for missing identity, unknown
  extent, stale rows, duplicate rows, cross-block rows, and unproven dynamic
  ranges.

Completion check:

- Focused tests cover known in-range access, negative or overflowing offset,
  access-size overflow, unknown extent, dynamic-array bounded range, and
  unproven dynamic range behavior.

## Step 5: Publish Provenance Verdicts to Prepared Consumers

Goal: make prepared `memory_accesses` consumers distinguish syntactic
base-plus-offset usability from proven object-range provenance.

Primary targets:

- `src/backend/prealloc/addressing.hpp`
- prepared lookup helpers and publication-plan consumers
- focused prepared edge/publication tests

Actions:

- Publish the structured provenance verdict alongside existing address
  usability fields.
- Update consumers that currently infer range safety from
  `can_use_base_plus_offset` to use the explicit provenance verdict.
- Keep target-specific prepared/prealloc acceptance out of the proof path.
- Preserve stale-row, duplicate-row, and cross-block prepared
  `memory_accesses` rejection.

Completion check:

- Prepared consumers can report or act on syntactic base-plus-offset usability
  separately from proven in-range provenance, without weakening existing
  fail-closed behavior.

## Step 6: Retire or Quarantine the Compatibility Bridge

Goal: remove or strictly quarantine the `allow_opaque_ptr_base && stored_type
== I8` bridge only after structured facts cover each required route.

Primary targets:

- the opaque pointer byte-offset admission rule
- focused opaque pointer and prepared memory-access tests

Actions:

- Compare every inventoried route against the structured carrier coverage from
  Steps 2 through 5.
- Replace the bridge for routes that now prove in-range access through
  structured facts.
- Keep explicit unknown/opaque compatibility verdicts for routes that cannot
  yet prove range.
- Do not downgrade supported-path expectations or convert fail-closed edges
  into compatibility acceptance.

Completion check:

- No route relies on `can_use_base_plus_offset` or `stored_type == I8` alone as
  object-range proof, and any remaining compatibility behavior is explicit and
  documented in `todo.md`.

## Step 7: Broader Validation and Closure Readiness

Goal: decide whether the source idea is complete and whether any adjacent
prepared-memory cleanup belongs in a new idea.

Actions:

- Run the supervisor-selected broader validation for the final provenance diff.
- Include focused proof for `backend_lir_to_bir_notes`,
  `backend_prepared_lookup_helper_test`,
  `backend_riscv_prepared_edge_publication_test`, and prepared memory-access
  subsets covering `PreparedAddressBaseKind::PointerValue` and
  `can_use_base_plus_offset`.
- Verify rendered type text is not primary range authority when structured
  layout or explicit extent facts exist.
- Verify missing identity, unknown extent, overflow, stale prepared rows, and
  unproven dynamic ranges remain fail-closed or explicitly unknown.
- Record any remaining adjacent prepared publication work in `todo.md` for
  supervisor lifecycle routing.

Completion check:

- The source idea acceptance criteria are satisfied, validation is recorded in
  `todo.md`, and the supervisor has enough evidence to request close or split
  follow-up work.
