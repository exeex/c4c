# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 constant initialized-definition packet now admits the exact
  coherent `is_const=true` plus `constant ` qualifier form beside the existing
  `is_const=false` plus `global ` form. Both remain restricted to non-extern,
  non-internal definitions with empty linkage visibility, exact structured
  type/source/rendered-type parity, and a nonempty opaque initializer payload.
- Nearby receipt coverage proves constant definitions preserve the typed object
  type, `is_const`, byte-exact opaque payload, and ordered structured initializer
  links. Both mismatched `is_const`/qualifier directions reject the whole module.

## Suggested Next

- Execute one bounded Step 3 internal initialized-definition packet. Admit only
  producer-evidenced `internal ` linkage forms with exact ordinary/constant
  qualifier pairing, prove typed `is_internal` receipt and transactional
  rejection of linkage/flag mismatches, and leave weak/visibility variants out.

## Watchouts

- Constant admission extends only `validate_module_surface`; the existing typed
  global/initializer receipt already preserves all required fields. External
  declarations remain unchanged, and internal, weak/visibility, aggregate, and
  special-type variants still fail closed.
- `init_text` is opaque receipt evidence, not parsed semantic or topology
  authority. `initializer_function_link_name_ids` alone supplies structured
  initializer references, and `LirGlobal.id` remains producer-default
  compatibility state rather than Raw-BIR identity.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof is sufficient for this importer admission and its
  nearby interface coverage.
