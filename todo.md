# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 const-pointer initialized-definition packet now admits the exact
  visibility-free producer form with `is_const=true`, structured `ptr`
  authority, `global ` qualifier, and a nonempty opaque initializer. Import
  preserves the typed Raw-BIR pointer, const state, identity, payload, and
  ordered initializer links without parsing initializer text.
- Nearby coverage distinguishes this producer form from the existing scalar
  const `constant ` definition and proves transactional rejection for a
  non-pointer const+`global ` mismatch, a pointer const+`constant ` mismatch,
  and a non-const ordinary pointer definition.

## Suggested Next

- Execute one bounded Step 3 string-pool or external-symbol completeness packet
  selected from the remaining runbook scope, keeping it independent of
  aggregate and flexible-special-type global support.

## Watchouts

- Pointer admission requires the structured `LirTypeRef` to lower exactly to
  Raw-BIR `TypeKind::Pointer`; `TypeSpec` and rendered text remain compatibility
  parity only. The compatibility path is limited to one ordinary pointer level
  and excludes references, arrays, pointer-to-array, inner-rank, and special
  function-pointer shapes. It is enabled only after the exact owned
  const-pointer producer row is established, so external, non-const, internal,
  and weak pointer rows remain fail-closed before existing coherence branches.
- Const pointer definitions are admitted only with empty linkage visibility and
  `global `; internal/weak pointer constants and `constant ` pointer forms remain
  fail-closed. Aggregate and flexible special types remain unsupported.
- `init_text` remains opaque receipt evidence; structured initializer function
  link IDs alone provide topology authority.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers importer admission, transactional
  rejection, typed global views, and initializer receipt preservation.
