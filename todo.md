# Current Packet

Status: Active
Source Idea Path: ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define and populate the selected memcpy typed authority

## Just Finished

- Plan Step 1 complete: the selected fixed-aggregate byval parameter
  materialization in `src/codegen/lir/hir_to_lir/lvalue.cpp` now publishes its
  `LirMemcpyOp::selected_authority` from
  `LirFunction::selected_memcpy_pointer_authority`. The descriptor carries
  destination/source `LirValueId`s, typed integer size, distinct object IDs,
  current-function owners, and live-at-site facts; display operands remain
  compatibility-only. Focused producer coverage asserts the published fields.

## Suggested Next

- Execute Step 2 only: enforce the selected row's verifier boundary for the
  newly published authority, including missing, identity, object/owner, type,
  size, and lifetime rejection with no display-text fallback.

## Watchouts

- This packet changes only the selected producer's optional descriptor; every
  other memcpy producer remains without selected authority. Step 2 must bind
  verifier checks to the selected descriptor and preserve the fail-closed
  producer path, without expanding to Raw-BIR/importer/receiver work.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log` passed: 5/5 backend
  tests, including `backend_lir_selected_pointer_authority`. The
  supervisor-selected subset was sufficient for this producer/schema packet;
  `test_after.log` is the proof log.
