# Current Packet

Status: Active
Source Idea Path: ideas/open/718_block_entry_publication_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize the prepared-to-BIR publication divergence

## Just Finished

- Completed Plan Step 1: localized the first prepared-to-BIR publication
  divergence without changing implementation.  The storage publication first
  becomes available in
  `find_prepared_current_block_entry_publication`: the successor-keyed
  `PreparedBlockEntryPublication` owns the move/bundle and destination ID, and
  the destination home owns the destination name ID.  The first divergence is
  the unconditional call from that helper to
  `attribute_block_entry_publication_proof_if_agreeing`.  When the query omits
  the BIR successor/destination proof (as the frame/stack contract fixture
  does), that helper changes the already-available prepared result to
  `MissingProof`.  `find_bir_block_entry_publication_identity` is downstream;
  it copies this status and the partial payload, then correctly refuses to set
  `available`, so it is not the first incorrect fact.
- Authority map: the prepared move bundle's
  `source_parallel_copy_successor_label` plus the query successor ID own the
  successor key; `PreparedMoveResolution::to_value_id` owns destination ID;
  the unique `PreparedValueHome` owns destination name ID; `PreparedNameTables`
  owns successor/name text; the unique regalloc row selected by destination ID
  owns destination type; and the BIR publication view result selected by the
  explicit successor block and destination value owns the PHI instruction and
  its index.  Successful attribution is owned only by agreement of all those
  facts: block label ID/text, destination name/type, block-entry kind, and BIR
  PHI index equal to the prepared bundle index.  The prepared record preserves
  the proof-view status and proof instruction index, but the MIR identity
  currently publishes the bundle index; equality checking makes the number
  agree while the BIR proof remains the semantic authority.
- Negative-path classification: absent proof pointers are `MissingProof`;
  no/invalid destination home or names are the existing typed missing or
  incomplete statuses; absent BIR publication is `ProofUnavailable`; duplicate
  BIR publication evidence is `ProofAmbiguous`; wrong successor ID/text,
  destination name, destination type, publication kind, or PHI/bundle index is
  `ProofMismatch`.  A stale successor owner is therefore a successor mismatch,
  a stale PHI coordinate is an instruction mismatch, and an available storage
  row with `block_entry_publication_proof_attributed == false` remains
  unattributed and must not become a BIR identity.
- General smallest-boundary repair rule: preserve storage readiness separately
  from semantic publication availability, and complete semantic identity at
  the common proof-attribution boundary from the explicit successor block and
  destination value.  Publish only the single agreeing proof row's successor,
  destination ID/name/type, and PHI instruction/index; otherwise return its
  precise typed unavailable status.  Never infer identity from source order,
  nearest PHI, display name alone, bundle order, or target-emission facts.

## Suggested Next

- Execute Plan Step 2 at the proof-attribution boundary, keeping prepared
  storage readiness distinct from attributed semantic availability and
  carrying the BIR-owned PHI identity through the adapter.

## Watchouts

- The frame/stack fixture currently manufactures successor/name/type/proof-bit
  fields after a lookup that already returned `MissingProof`; do not repair the
  fixture with another manual completion or by forcing the status/proof bit.
- `prepared_printer/value_locations.cpp` also requests attributed proof without
  supplying regalloc, so any repair must classify that caller deliberately
  rather than weakening the common agreement check.
- Do not absorb prepared-call, join-source, edge-publication, or target
  materialization work, and do not select `%join.arg`, value 71, or instruction
  zero specially.

## Proof

- `git diff --check -- todo.md` (supervisor-selected Step 1 documentation
  proof): passed.  No build/test run or `test_after.log` was requested for this
  evidence-only packet.
