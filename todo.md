# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 accumulated module-surface checkpoint now proves that one
  imported module can preserve an admitted named structured declaration and
  symbol domain, ordered string data, a link-backed external declaration,
  fallback and named-aggregate globals, ordered initializer links, and
  specialization metadata through stable typed Raw BIR views.
- The same combined module reaches `FoundationVerifier`, publishes Canonical
  BIR, and two distinct late invalid cross-references (initializer and
  specialization links) reject with their exact existing typed errors in both
  routes without publishing a partial result.

## Suggested Next

- Execute one bounded Step 3 flexible-array-member global receipt packet for
  the actual non-extern direct-struct definition emitted by `lower_global`
  with literal aggregate `llvm_type` / `llvm_type_ref`; preserve its typed
  object facts, opaque initializer payload, and ordered initializer links, with
  neighboring positive/negative Raw and Canonical transactional proof.

## Watchouts

- Step 3 is incomplete while this producer-valid literal aggregate global row
  remains closed; do not treat the accepted accumulated checkpoint as a
  completion decision.
- Keep the packet specific to structured producer evidence rather than opening
  arbitrary literal aggregates: rendered text remains parity or opaque
  payload, never parsed semantic authority, and other closed global shapes
  require their own producer evidence before admission.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers the combined admitted Step 2-3 module
  surface, verifier reachability, Canonical publication, stable typed
  cross-references, and Raw/Canonical transactional rejection for two distinct
  late invalid facts.
