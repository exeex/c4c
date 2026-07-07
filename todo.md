Status: Active
Source Idea Path: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Prepared Authority Taxonomy

# Current Packet

## Just Finished

Step 2: Define The Prepared Authority Taxonomy completed as a taxonomy-only
packet.

Changed files:
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/prepared_object_traversal.hpp`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`

Implemented:
- Added `PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` with stable
  spelling `stack_destination_register_fan_in`.
- Added
  `PreparedObjectMoveBundleConsumerStatus::
  UnsupportedNonParallelMultiSourceStackDestinationAuthority` and matching
  diagnostic category/name to keep explicit unsupported or unknown fan-in
  authority distinct from missing authority.
- Adjusted the prepared move-bundle classifier so missing authority still
  reports `AmbiguousNonParallelMultiSourceStackDestination`, while explicit or
  unknown non-parallel multi-source stack-destination authority remains
  fail-closed.
- Added focused taxonomy/name coverage and RV64 rejection coverage proving the
  new authority spelling is representable but does not make arbitrary
  multi-source stack-destination fan-in emit.

## Suggested Next

Delegate Step 3. Publish `StackDestinationRegisterFanIn` only from the
producer/classifier surface that can prove the legal fan-in shape, and keep
ambiguous or unsupported shapes on the existing fail-closed diagnostics.

## Watchouts

- Keep idea 585 inactive; it is documentation/research and says activation is
  out of scope unless requested later.
- Do not accept value-id-, filename-, function-, block-, offset-, or
  diagnostic-string-specific authority.
- Preserve fail-closed behavior for missing, unknown, unsupported, and
  genuinely ambiguous stack-destination fan-in.
- The new authority enum is representable, but Step 2 deliberately does not
  make RV64 accept arbitrary non-parallel stack-destination fan-in.
- `value_locations.hpp` did not need new fields for this packet; the existing
  bundle and move evidence already carries function, phase, authority,
  source/destination value IDs, homes via lookups, and producer location.
- Do not make RV64 infer legality from the current diagnostic path; the next
  implementation packet should publish producer authority in prealloc first,
  then a later RV64 packet can consume it.
- The existing diagnostic producer is RV64, but the missing authority itself is
  exposed by the prepared object move-bundle classifier. Keep that distinction
  explicit in the taxonomy packet.
- Existing positive stack-destination materialization is select-publication
  backed; it should not be generalized by shape alone into arbitrary
  non-parallel bundles.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the proof output; CTest reports
`Total Test time (real) = 2.12 sec`.
