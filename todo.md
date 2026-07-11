# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate prepared lookup attribution

## Just Finished

- Completed Step 4: block-entry lookup attribution now consumes the named
  `BirPublicationView` fact and binds it to prepared successor identity and the
  prepared publication cursor.
- Removed Route 4 query aliases and route-index/status aliases from the public
  prepared value-location record while preserving prepared homes, registers,
  publication linkage, and instruction-index authority.

## Suggested Next

- Execute Step 5: align remaining prepared records, printers, and store-source
  proof with ownership-named evidence.

## Watchouts

- Prepared availability remains independent of BIR attribution; missing,
  unavailable, ambiguous, wrong-type, wrong-successor, and cursor-mismatched
  named evidence deliberately leaves the prepared lookup unattributed.
- Do not reintroduce route-numbered compatibility fields when updating printer
  terminology in Step 5.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`.
- Result: 329/329 backend tests passed, including available block-entry
  attribution and missing, ambiguous, wrong-type, wrong-successor, and
  cursor-mismatched rejection; canonical proof log is `test_after.log`.
