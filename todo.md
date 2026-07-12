# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Disposition the ALU return-chain authority gap

## Just Finished

- Plan Step 2.1 disposition: the existing traversal-attached common typed
  consumer API does not express the ALU return-chain relation, so no consumer
  migration was made. `PreparedObjectTraversalEvent` identifies one traversal
  site and its attached move bundle, while
  `PreparedObjectMoveBundleConsumerClassification` exposes that bundle,
  per-source freshness, and optional stack fan-in authority only. It does not
  query or return the chain's terminal return-ABI value home, the first
  successor ALU's non-chain operand home, or the cross-instruction relation
  between successive value moves and scalar producers that `alu.cpp` needs.

## Suggested Next

- Have the supervisor disposition the documented common-query contract gap;
  adding that producer/query authority would cross this packet's ownership and
  the current idea's no-redesign boundary.

## Watchouts

- `find_prepared_return_chain_facts` locally reconstructs all missing parts by
  walking `BeforeInstruction` move bundles, resolving successor homes, querying
  same-block scalar producers, checking instruction adjacency and chain-operand
  identity, and extracting the first non-chain operand. Migrating only its
  one-event move lookup to the typed classifier would leave the cross-event
  reconstruction intact and would not satisfy the source idea.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1`: build passed, but
  the existing backend suite failed 52 of 400 tests across RISC-V, common MIR,
  AArch64, prepared-BIR, and x86 families. No implementation files changed in
  this evidence-only packet; full failure details are in `test_after.log`.
