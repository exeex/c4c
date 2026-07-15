# Current Packet

Status: Active
Source Idea Path: ideas/open/788_lir_phi_incoming_successor_occurrence_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Record the exact parent handoff

## Just Finished

- Plan 788 Step 3: made the LIR verifier require every PHI incoming to select
  one valid typed predecessor terminator occurrence to its destination, reject
  duplicate selections, and exactly cover all destination-edge occurrences;
  added typed conditional/switch parallel-edge positives and malformed cases.

## Suggested Next

- Plan 788 Step 4: record the exact parent handoff for the native occurrence
  carrier, producer population, verifier guarantees, coverage, and return to
  parent 734 Step 7.25.

## Watchouts

- Occurrence IDs are interpreted only against the typed predecessor terminator;
  conditional true/false and switch default/cases remain distinct even when
  their destinations are equal. Nonselected PHI producers remain absent.
- Keep 734's `006d79aaf` accepted partial receiver intact. Do not edit Raw-BIR,
  importer/container/verifier, lowering, or later LIR families.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1 passed). The
  delegated packet explicitly prohibited writing `test_after.log`.
