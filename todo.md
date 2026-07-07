Status: Active
Source Idea Path: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Legal Producer Authority

# Current Packet

## Just Finished

Step 3: Publish Legal Producer Authority completed.

Changed files:
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`

Implemented:
- Added a producer-side publication pass that stamps
  `PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` on a
  before-instruction move bundle and its moves only when the bundle is a
  select-result stack-destination fan-in and the same select is indexed as a
  `SelectMaterialization` source producer.
- Kept publication fail-closed for same-shaped non-select producers, missing
  producer evidence, wrong result value IDs, non-stack destinations, non-move
  operations, unknown homes, and incomplete stack/register source mixes.
- Hooked the publication pass into prepared contract publication after
  source-publication planning and before select-carrier identity publication.
- Added focused RV64/prepared object-emission coverage proving legal producer
  publication and non-select producer non-publication without broadening
  arbitrary RV64 stack-destination fan-in acceptance.

## Suggested Next

Delegate Step 4. Teach RV64 to consume
`StackDestinationRegisterFanIn` only from the prepared authority facts now
published by prealloc, and keep missing, unknown, non-select, and contradictory
authority fail-closed.

## Watchouts

- Keep idea 585 inactive; it is documentation/research and says activation is
  out of scope unless requested later.
- Do not accept value-id-, filename-, function-, block-, offset-, or
  diagnostic-string-specific authority.
- Preserve fail-closed behavior for missing, unknown, unsupported, and
  genuinely ambiguous stack-destination fan-in.
- Step 3 stamps both the bundle and each move with
  `StackDestinationRegisterFanIn`; existing tests that hand-set only bundle
  authority still prove arbitrary fan-in stays unsupported.
- RV64 consumption is still intentionally not broadened in this packet.
- The producer-side legal shape currently requires a before-instruction select
  result, all move destinations matching that select result value ID, at least
  two register sources, one stack source, and a matching
  `SelectMaterialization` source producer record.
- Step 4 should consume the authority facts directly; it should not rediscover
  legality from CFG shape, raw names, offsets, diagnostics, or local RV64-only
  heuristics.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the proof output; CTest reports
`Total Test time (real) = 2.18 sec`.
