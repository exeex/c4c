# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Implement direct publication-source identity authority

## Just Finished

- Completed plan Step 6 by making direct named operand authority validate an
  available publication's exact source plus its id/name/home consistency and,
  when producer evidence is present, the producer's preserved result identity.
- Strengthened the focused probe for dependency, producer-conflicting rewrite,
  missing id/home, conflicting name/home, and non-consumer fail-closed cases.

## Suggested Next

- Execute plan Step 7 as the bounded transitive producer-closure packet without
  changing the direct identity query or selecting a producer by instruction
  order.

## Watchouts

- Direct identity intentionally does not traverse producer operands; Step 7
  owns closure. Legacy publications without producer evidence retain their
  existing id/name/home validation, while supplied producer evidence must agree
  exactly with the preserved source.
- The backend subset still has the known Step 7 transitive-closure and Step 9
  stable-key focused gaps; no unrelated regression remains in this packet.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
- Build passed; 322/324 backend tests passed. The direct publication identity
  probe is green. Only the known transitive dependency closure and stable-key
  composition focused probes remain red. Proof log: `test_after.log`.
