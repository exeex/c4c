# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 specialization-metadata packet now gives every current
  `LirSpecEntry` an ordered typed `SpecializationId`, structured Raw-BIR record,
  builder receipt, immutable module view, verifier rule, and importer wiring.
- Import preserves exact `spec_key`, `template_origin`, and `mangled_name`
  strings plus the resolved typed link identity. Malformed fields, unresolved
  or mismatched links, and duplicate semantic or link identities reject before
  publication, while failed builder receipts append no partial state.

## Suggested Next

- Execute one bounded remaining Step 3 global, external-symbol, or initializer
  completeness packet selected from the runbook, keeping aggregate and
  flexible-special-type global support separate.

## Watchouts

- Producer evidence defines specialization uniqueness by both deduplicated
  function `LinkNameId` and the `(template_origin, spec_key)` semantic pair;
  Raw-BIR indexes and verifies both without globally forbidding legitimate key
  reuse across different template origins.
- The source link spelling must equal `mangled_name`; this matches the current
  printer's authoritative-link behavior and prevents rendered metadata from
  becoming identity evidence.
- Intrinsic requirements and remaining aggregate/flexible-special-type global
  shapes remain unsupported and were not widened by this packet.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers ordered structured views, exact importer
  admission, builder rejection without partial append, verifier reachability,
  uniqueness enforcement, and whole-module transactional rejection.
