# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 initialized-global/initializer packet added typed lossless
  `GlobalInitializer` receipt for the coherent ordinary initialized-definition
  subset. Raw-BIR now preserves `LirGlobal::init_text` byte-for-byte as an
  opaque payload and preserves ordered, duplicate-retaining function references
  as epoch-owned `LinkNameId`s resolved only through the module link-name table.
- Builder resolution rejects invalid or dangling initializer references before
  append, importer publication remains module-transactional, and foundation
  verification enforces extern/definition initializer coherence plus every
  initializer link's epoch and link-name domain. Initializer-free external
  global coverage remains admitted and unchanged.

## Suggested Next

- Execute only the next bounded Step 3 constant initialized-definition packet.
  Reuse the typed initializer receipt while requiring exact agreement between
  `is_const`, the structured type, and the `constant ` qualifier; do not absorb
  weak/visibility or aggregate special-type work into that packet.

## Watchouts

- The admitted definition subset is deliberately restricted to non-internal,
  non-const, empty-linkage `global ` rows with exact `TypeSpec`/`LirTypeRef`
  parity and a nonempty opaque initializer payload. Weak/visibility forms and
  non-scalar special type parity still lack current typed authority and remain
  fail-closed.
- `init_text` is opaque receipt evidence, not parsed semantic or topology
  authority. `initializer_function_link_name_ids` alone supplies structured
  initializer references, and `LirGlobal.id` remains producer-default
  compatibility state rather than Raw-BIR identity.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The supervisor reran the exact fresh build plus matching `^backend_` before
  and after commands; canonical `test_before.log` and `test_after.log` both
  record 4/4 backend tests passing. The documented non-decreasing monotonic
  guard passed with an equal CTest count because nearby initializer coverage
  was added inside the existing interface test binary.
