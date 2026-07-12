# Current Packet

Status: Active
Source Idea Path: ideas/open/718_block_entry_publication_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Restore exact semantic publication identity

## Just Finished

- Completed Plan Step 2: added a proof-bearing
  `find_bir_block_entry_publication_identity` overload that publishes the exact
  BIR-owned successor block, destination value, PHI instruction, PHI payload,
  instruction index, destination name/type, and prepared destination ID only
  after all prepared attribution fields agree with the explicit BIR proof.
- Preserved typed fail-closed results: missing explicit proof is `MissingProof`,
  an absent BIR PHI is `ProofUnavailable`, and inconsistent successor,
  destination, type, attribution, or instruction coordinates are
  `ProofMismatch`.  The existing target-facing overload and prepared-call,
  join-source, edge-publication, and target materialization paths were not
  changed.
- Repaired the focused fixture to construct real names/regalloc/BIR proof
  evidence and removed its manual successor/name/type/proof-bit completion.

## Suggested Next

- Execute Plan Step 3 by adding nearby positive and negative shapes for wrong
  successor, wrong destination/type, stale PHI coordinate, duplicate proof,
  and unattributed prepared evidence through the proof-bearing overload.

## Watchouts

- The compatibility overload remains for existing target consumption; Step 3
  proofs should exercise the explicit proof-bearing overload when asserting
  exact BIR pointer identity.
- Do not use Route 4 source order as semantic authority for duplicate-PHI
  coverage; duplicate evidence must remain attributed upstream as
  `ProofAmbiguous` and fail closed before publication.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$' | tee
  test_after.log`: passed (1/1); the supervisor-selected focused proof was
  sufficient for this packet and is recorded in `test_after.log`.
