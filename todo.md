Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define the Provenance and Byte-Range Carrier

# Current Packet

## Just Finished

Step 2: Define the Provenance and Byte-Range Carrier completed.

Introduced passive structured memory provenance carriers in BIR/prepared
addressing without changing acceptance behavior:

- `bir::MemoryAccessProvenance` now represents base identity, object extent,
  requested byte range with explicit overflow/end-availability status, layout
  authority, dynamic-array facts, and an explicit `UnknownCompatible` range
  verdict.
- `bir::MemoryAddress`, Route 3 memory records, lowering-local
  `PointerAddress`, and `prepare::PreparedAddress` now carry the provenance
  object.
- BIR-to-prepared conversion copies existing provenance through and, when no
  route has populated base identity yet, fills neutral base identity and
  requested-range facts from the already accepted BIR memory address.
- Addressed pointer load/store lowering populates pointer-value base identity,
  requested range, neutral `UnknownCompatible` verdict, and available dynamic
  element count/stride facts from existing `PointerAddress` state.
- The base-identity enum explicitly reserves stable cases for formal
  parameters, byval parameters, sret parameters, and unknown runtime bases in
  addition to local slot, global symbol, pointer value, and string constant
  identities.
- Requested byte-range construction now uses a defensive BIR helper that marks
  overflowed ranges instead of relying on unchecked signed end arithmetic.
- `can_use_base_plus_offset` consumers are unchanged; no target-specific
  prepared/prealloc policy consults the new carrier yet.

Deferred to later population steps:

- object extent sizes/completeness remain unknown unless a later route proves
  them
- layout authority remains unknown unless a later route proves structured,
  scalar, byte-storage, rendered fallback, or opaque compatibility authority
- formal/byval/sret and unknown-runtime-base identity kinds are represented in
  the carrier but not populated yet because this neutral Step 2 slice does not
  change route classification or acceptance policy
- dynamic selected index and bounded/unbounded verdicts are represented but not
  broadly populated in this neutral carrier slice

## Suggested Next

Populate proven object extent and layout-authority facts for one source route
at a time, keeping `can_use_base_plus_offset` separate from range proof and
preserving the existing opaque pointer compatibility behavior until structured
verdicts cover all required routes.

## Watchouts

- The new carrier is intentionally passive. Do not gate target lowering,
  publication, or lookup behavior on it until a later step explicitly defines
  the verdict semantics.
- `UnknownCompatible` is a neutral compatibility verdict, not proof that the
  requested byte range is in bounds.
- `MemoryByteRange::end` is meaningful only when `end_available` is true; an
  overflowed requested range must be treated as unproven until a later route
  defines fail-closed semantics.
- `MemoryDynamicArrayFacts::index` is represented but currently defaults empty
  for the addressed pointer route because `PointerAddress` only carried count
  and stride in this slice.
- Avoid adding equality/lookup/printer participation for provenance fields
  until the supervisor selects a route that deliberately publishes and tests
  those semantics.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
