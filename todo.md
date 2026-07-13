# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 global-visibility packet now losslessly receives the current
  producer's exact default, hidden, and protected visibility suffixes as the
  closed typed `SymbolVisibility` field on every already-supported global
  declaration and definition shape.
- Import decodes visibility independently from existing linkage, qualifier,
  type, and initializer authorities. Unknown, repeated, or contradictory
  spellings reject the whole module, while default rows retain `Default` and
  typed views distinguish `Hidden` and `Protected`.

## Suggested Next

- Execute one bounded remaining Step 3 global, external-symbol, or initializer
  completeness packet selected from the runbook, keeping aggregate and
  flexible-special-type global support separate.

## Watchouts

- `make_linkage_vis` appends visibility uniformly after the structured linkage
  prefix, including for const-pointer globals. The existing ordinary
  const-pointer row therefore accepts visibility variants without admitting
  weak const-pointer or new pointer families.
- Visibility remains orthogonal to `is_internal`, weak/external classification,
  qualifier, initializer, and type compatibility checks; it does not authorize
  any new linkage or definition shape.
- Function visibility, aggregates, flexible-special-type globals, non-const
  pointer definitions, and new initializer topology remain out of scope.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers typed default/hidden/protected views,
  external, weak-external, internal, ordinary, and const-pointer neighbors,
  verifier rejection of invalid enum values, exact spelling validation, and
  Raw/Canonical whole-module rollback for malformed rows.
