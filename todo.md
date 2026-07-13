# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 internal initialized-definition packet now admits only the exact
  producer forms: non-extern `is_internal=true`, `internal ` linkage, nonempty
  opaque initializer, and either `is_const=false` plus `global ` or
  `is_const=true` plus `constant `. Structured source/type-text parity remains
  mandatory before receipt.
- Foundation verification now accepts the already-typed `is_internal` flag
  while retaining all identity, index, type, alignment, initializer-coherence,
  payload/link-domain, and link-identity obligations. Nearby coverage proves
  typed internal ordinary/constant views and transactional rejection of both
  flag/linkage mismatch directions and both qualifier mismatch directions.

## Suggested Next

- Execute one bounded Step 3 visibility-free weak initialized-definition
  packet. Add only the minimal typed Raw-BIR linkage authority needed to retain
  weak identity losslessly, admit exact scalar producer forms, and prove weak
  versus ordinary/internal distinctions; leave visibility-decorated and
  `extern_weak` forms out.

## Watchouts

- The verifier change removes only the obsolete blanket internal rejection;
  existing malformed internal staging still fails through independent void-type
  and invalid-alignment rules, and extern/initializer incoherence remains
  independently covered. External, weak/visibility-decorated, aggregate, and
  special-type variants remain unchanged and fail closed.
- `init_text` is opaque receipt evidence, not parsed semantic or topology
  authority. `initializer_function_link_name_ids` alone supplies structured
  initializer references, and `LirGlobal.id` remains producer-default
  compatibility state rather than Raw-BIR identity.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers the shared verifier seam plus the importer
  admission and nearby interface coverage.
