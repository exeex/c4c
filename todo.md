# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove routed-operand and immediate-destination authority

## Just Finished

- Step 3 added and registered
  `backend_prealloc_current_block_routed_operand_authority_test.cpp`.
- The focused query now gives `BinaryInst`, `CastInst`, and `SelectInst`
  operands explicit prepared-source authority, while immediate sources require
  complete destination-home identity and authorize that destination.
- Unrelated operands, conflicting source or destination metadata, and source
  identity rewriting fail closed; no AArch64 behavior or expectations changed.

## Suggested Next

- Execute the bounded Step 4 all-applicable-edge invariance packet, keeping it
  independent of AArch64 consumption.

## Watchouts

- Routed-operand authority deliberately does not decide predecessor or
  parallel-edge agreement; Step 4 owns that complete fact-family invariant.
- Keep the prepared source identity pointer distinct from the authoritative
  destination returned for immediate sources.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: 319/319 backend tests passed, including
  `backend_prealloc_current_block_routed_operand_authority`.
- Canonical proof log: `test_after.log`.
