# Current Packet

Status: Active
Source Idea Path: ideas/open/727_common_prepared_return_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce and attach typed return-chain authority

## Just Finished

- Completed Plan Step 2: added the typed return-chain status, query, link,
  relation, and classification API in common prepared traversal code.
- `classify_prepared_object_return_chain` now composes authenticated prepared
  value homes, indexed move bundles, same-block scalar producers, per-link
  freshness authority, and the terminal before-return ABI move/binding.
- Complete classifications preserve ordered link identities, operand roles,
  the optional first named non-chain operand home, and target-neutral terminal
  bank/placement. Missing, stale, ambiguous, inconsistent, unsupported,
  non-adjacent, wrong-operand, incomplete, and cyclic evidence fails closed
  with no consumable relation.
- `make_prepared_object_function_traversal` accepts optional common names and
  function lookups and attaches the classification to each starting
  `Instruction` event; existing callers remain source-compatible.
- Acceptance corrections copy scalar-producer evidence into each link instead
  of retaining a pointer into a temporary query result, and reject terminal
  ABI evidence unless at least one authenticated link has been accumulated.
- Audited remaining relation pointers: they refer only to immutable BIR,
  prepared value-location storage, or lookup-owned storage, while freshness
  and producer evidence that originates in local query results is value-copied.

## Suggested Next

- Execute Plan Step 3: add focused common contract proof for at least two valid
  chain shapes and the negative matrix.

## Watchouts

- Focus the next packet on common contract tests only; target consumption is
  outside this runbook step.
- `review/aarch64_step2_route_review.md` pre-existed this executor packet and
  remains unmodified/untracked.

## Proof

- Green exact delegated proof after one transient compiler OOM retry:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_object_consumer_contract$'`.
  The focused test passed 1/1. CTest output is preserved in `test_after.log`;
  this proof is sufficient for the Step 2 common authority packet.
