# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits the producer-valid non-extern direct `TB_STRUCT`
  flexible-member global row when its required `llvm_type_ref` is an unkeyed,
  well-formed brace-delimited literal Struct exactly matching `llvm_type`.
- Nearby proof preserves byte-exact typed spelling with invalid
  `StructNameId`, link-backed identity, linkage/const/visibility/alignment
  facts, opaque initializer bytes, and ordered initializer links through Raw
  verification and Canonical publication; literal unions, missing or
  mismatched mirrors, literal externs, and TypeSpec array/pointer neighbors
  reject transactionally in both routes.

## Suggested Next

- Execute one bounded Step 3 producer-valid TypeSpec array-global receipt
  packet. `lower_global` emits ordinary array declarations/definitions through
  its generic path with structured TypeSpec array facts and rendered LLVM type,
  while `lower_constant_type` currently rejects every nonzero `array_rank`.

## Watchouts

- Literal aggregate admission is intentionally limited to initialized,
  non-extern, direct `TB_STRUCT` rows whose typed unkeyed Struct mirror exactly
  matches the brace-delimited spelling. Named structs/unions still require a
  resolvable `StructNameId`; literal unions and all TypeSpec array/pointer
  aggregate neighbors remain closed.
- For the suggested array packet, derive authority from TypeSpec and any
  producer-provided typed evidence; do not parse rendered LLVM type or opaque
  initializer payload to infer element type, extent, or topology.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers typed literal-Struct receipt,
  `FoundationVerifier` reachability, Canonical publication, exact object-fact
  preservation, and Raw/Canonical transactional rejection of neighboring
  unsupported global shapes.
