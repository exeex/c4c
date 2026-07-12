# Current Packet

Status: Active
Source Idea Path: ideas/open/710_rv64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate RV64 object intent

## Just Finished

- Completed Plan Step 3's directly implicated RV64 object-intent cleanup.
  Removed the obsolete Route 3/5 oracle test and its route-derived BIR fixture
  after the RV64 emitter's final executable dependency had already migrated to
  ownership-named prepared publication, move, and memory-access facts. The
  remaining focused tests preserve prepared positive, structured-evidence, and
  fail-closed coverage without agreement rows or expectation weakening.

## Suggested Next

- Execute Plan Step 4's semantic retirement search and broader matching RV64
  regression comparison.

## Watchouts

- The focused object-emission test retains its pre-existing failure family;
  compare `test_after.log` against the canonical supervisor-owned baseline
  before accepting the slice. The prepared edge-publication test is green and
  the scoped Route 3/5 vocabulary search is empty.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_(riscv_prepared_edge_publication|riscv_object_emission))$'`:
  build passed; `backend_riscv_prepared_edge_publication` passed;
  `backend_riscv_object_emission` failed with the existing call, branch
  stack-load authority, frame-slot-address argument, and abort-runtime
  baseline family. Combined output is preserved in `test_after.log`; supervisor
  baseline comparison remains required.
