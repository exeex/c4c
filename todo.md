# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 named-aggregate global packet now imports actual ordinary struct
  and union `lower_global` definitions whose `llvm_type_ref` carries a
  resolvable `StructNameId`, preserving typed Struct identity, object/linkage
  facts, opaque initializer payload, and ordered initializer links.
- Receipt requires a direct aggregate `TypeSpec`, exact rendered parity, and a
  matching structured declaration; absent, unresolved, mismatched, literal,
  array, aggregate-extern, and pointer-to-aggregate shapes reject Raw and
  Canonical import transactionally.

## Suggested Next

- Execute one bounded Step 3 accumulated module-level checkpoint packet proving
  the admitted Step 2-3 named types, strings, externs, globals, initializers,
  and specialization metadata coexist and roll back transactionally.

## Watchouts

- `llvm_type_ref` and its resolvable `StructNameId` are aggregate semantic
  authority; `llvm_type` and `init_text` remain parity/opaque payload and are
  never parsed into type identity or initializer topology.
- Flexible-array-member globals intentionally use unkeyed literal types in the
  producer and remain closed, as do literal aggregates, arrays, aggregate
  externs, and pointer-to-aggregate shapes.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers producer-shaped named struct and union
  global receipt, FoundationVerifier reachability, preserved identity/object
  facts, and Raw/Canonical rollback for all neighboring closed shapes.
