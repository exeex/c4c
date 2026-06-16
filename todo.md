Status: Active
Source Idea Path: ideas/open/290_gate_quarantined_opaque_compatibility_memory_accesses.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Choose and Specify One Policy Surface

# Current Packet

## Just Finished

Step 2: Choose and Specify One Policy Surface completed as a policy-spec
packet.

Chosen Step 3 policy surface:

- Implement lowerer-time rejection in the target-independent addressed pointer
  paths:
  `BirFunctionLowerer::try_lower_addressed_pointer_load` and
  `BirFunctionLowerer::try_lower_addressed_pointer_store` in
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`.
- The gate is intentionally before prepared `memory_accesses` publication.
  Quarantined addressed pointer rows should fail closed instead of being
  emitted as source-memory rows that later prepared/prealloc consumers must
  diagnose.
- Do not use the prepared/publication consumer gate as the first
  behavior-changing surface in Step 3. The existing flattened
  `source_memory_layout_authority` field and strict prepared matcher remain
  relevant proof coverage, but no new flattened fact is required for this
  chosen lowerer-time route.

Exact predicate:

- Compute the existing `addressability` with
  `classify_scalar_subobject_addressability(...)` exactly as the load/store
  helpers do today.
- Reject only when
  `addressability == ScalarSubobjectAddressability::OpaqueCompatibility`.
- Keep the existing rejection for
  `!is_scalar_subobject_addressable(addressability)`, but do not broaden it
  into a policy gate for `UnknownCompatible`,
  `can_use_base_plus_offset`, unknown extent, or any prepared flattened field.
- This predicate is the explicit opaque-compatibility metadata surface because
  it is the enum that currently causes
  `pointer_value_memory_provenance_with_layout_authority` to publish
  `layout_authority = MemoryLayoutAuthorityKind::OpaqueCompatibility` and
  `range_verdict = MemoryRangeVerdict::UnknownCompatible`.

Expected outcomes:

- Structured proven rows:
  `ScalarSubobjectAddressability::Accepted` addressed pointer load/store rows
  continue to lower, continue to publish non-opaque provenance, and remain
  eligible for prepared source-memory publication. Examples include same-type
  scalar access, byte-wise scalar object representation access, and aggregate
  byte-storage access that proves the requested range in bounds.
- Quarantined rows:
  any addressed pointer load/store row whose scalar-subobject classification is
  `OpaqueCompatibility` fails closed at lowering time. Step 3 should cover both
  the loaded-pointer store route and the casted-byte-pointer load/store route
  if both are affected by the chosen fixture shape.
- Stale rows:
  no new stale-row matching field is required for the chosen lowerer gate.
  Existing prepared strict matching must remain unchanged, including comparison
  of `source_memory_layout_authority`, `source_memory_range_verdict`,
  dynamic-array verdict, byte range, base identity, address space, volatility,
  and `source_memory_can_use_base_plus_offset`.
- Duplicate rows:
  prepared duplicate-row handling must not be weakened. Since quarantined rows
  are rejected before publication, Step 3 must not add a fallback that accepts
  a duplicate prepared row after an addressed pointer opaque-compatibility
  lowering failure.
- Cross-block rows:
  cross-block prepared lookup rejection must remain position/block scoped.
  Step 3 must not add any cross-block search or target-specific prepared
  bypass to recover quarantined rows after the lowerer gate rejects them.

Focused tests to edit/add:

- Edit `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.
- Update the current opaque-compatibility acceptance fixture
  `expect_casted_byte_pointer_i32_update_uses_pointer_base` so the same
  semantic route now expects fail-closed lowering for the casted `i8*` to
  wider `i32` addressed load/store path. The failure check should assert a
  lowering failure note for the function rather than silently accepting
  pointer-base memory accesses.
- Keep or add a paired structured accepted-row test in the same file. The
  existing `expect_loaded_pointer_addressed_store_uses_pointer_base` should
  continue to prove a structured non-opaque addressed pointer store lowers
  through `MemoryAddress::BaseKind::PointerValue`; if Step 3 changes the
  fixture set, preserve an equivalent accepted structured row check.
- Keep the generic requested-range verdict test
  `expect_memory_access_requested_range_verdicts_use_complete_extent` unchanged
  so `UnknownCompatible` from unknown extent remains distinct from the
  explicit opaque-compatibility gate.
- Do not edit prepared/prealloc tests unless the code-changing packet
  unexpectedly touches prepared publication/matching. If it does, the minimum
  focused prepared coverage is `backend_prepared_lookup_helper` for strict
  `source_memory_layout_authority` stale-row mismatch behavior.

## Suggested Next

Step 3 implementation packet: edit
`src/backend/bir/lir_to_bir/memory/provenance.cpp` and
`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` to reject
`ScalarSubobjectAddressability::OpaqueCompatibility` in
`try_lower_addressed_pointer_load` and
`try_lower_addressed_pointer_store`, while preserving the structured accepted
addressed pointer store test.

## Watchouts

- The behavior predicate is the explicit
  `ScalarSubobjectAddressability::OpaqueCompatibility` result, not
  `MemoryRangeVerdict::UnknownCompatible` by itself.
- Do not delete or weaken `pointer_value_memory_provenance_with_layout_authority`;
  non-rejected non-opaque addressed pointer rows still need provenance
  publication, and prepared tests still cover flattened source-memory fields.
- Expect existing acceptance tests around casted byte-pointer updates to change
  behavior. That should be a fail-closed test update, not an unsupported-path
  downgrade or a target-specific exception.
- If the implementation chooses to emit a more specific diagnostic message,
  keep it tied to the opaque-compatibility lowerer gate and avoid implying that
  every `UnknownCompatible` range is rejected.
- Leave prepared stale-row, duplicate-row, and cross-block rejection untouched
  unless a failing test proves the lowerer gate accidentally affected them.

## Proof

No build or test run; proof was explicitly not required for this
policy-spec `todo.md` update. No root-level logs were created.

Required Step 3 proof ladder after code/test edits:

- Build: `cmake --build build --target backend_lir_to_bir_notes_test`
- Focused test: `ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure`
- Broader prepared safety, if Step 3 touches prepared publication/matching or
  the supervisor wants acceptance-policy confidence:
  `cmake --build build --target backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test`
  followed by
  `ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer)$' --output-on-failure`
