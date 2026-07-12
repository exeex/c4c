# Current Packet

Status: Active
Source Idea Path: ideas/open/720_block_entry_publication_proof_evidence_multiplicity.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Restore conservative production compatibility

## Just Finished

- Plan Step 6.1 restored conservative production compatibility: the legacy
  pointer overload no longer manufactures attributed availability from a
  display-name/type scan, preserves same-name/same-type duplicate
  `ProofAmbiguous` behavior, and AArch64 now requires a positive identity.

## Suggested Next

- Execute Plan Step 6.2: validate the complete modeled Route4 instruction
  coordinate, including `instruction_owner_label_id`.

## Watchouts

- Do not hand back to idea 718 until Steps 6.1 through 6.5 and the final audit
  are complete.
- The legacy pointer overload remains deliberately conservative: zero matching
  PHIs is unavailable, one is an unattributed mismatch, and duplicates are
  ambiguous; only the typed classification boundary may report availability.
- Keep Route4 authoritative for the complete modeled coordinate, including
  `instruction_owner_label_id`.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$' | tee
  test_after.log`; `test_after.log` is the canonical focused proof log.
