# Current Packet

Status: Active
Source Idea Path: ideas/open/788_lir_phi_incoming_successor_occurrence_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish native per-incoming occurrence authority

## Just Finished

- Plan 788 Step 2: added the native numeric `LirSuccessorOccurrenceId` carrier
  to `LirPhiIncoming`, populated the selected ternary, logical, AArch64-vaarg,
  and AMD64-vaarg PHIs from their direct typed CFG exits, and added structural
  coverage for those producers plus distinct parallel conditional/switch IDs.

## Suggested Next

- Plan 788 Step 3: make the verifier reject missing, invalid, foreign,
  mismatched, duplicate, and multiplicity-incoherent PHI successor-occurrence
  authority before downstream use.

## Watchouts

- The selected producers all join through direct branches (occurrence zero);
  conditional true/false and switch default/cases retain separate typed IDs for
  the Step 3 verifier contract. Nonselected PHI producers remain absent.
- Keep 734's `006d79aaf` accepted partial receiver intact. Do not edit Raw-BIR,
  importer/container/verifier, lowering, or later LIR families.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(frontend_lir_call_type_ref|backend_)' >
  test_after.log 2>&1; status=$?; cat test_after.log; exit $status`.
  The selected CTest registry run executed `backend_` (5/5 passed); the log is
  preserved at `test_after.log`.
