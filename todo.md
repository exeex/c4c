# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Run the acceptance proof

## Just Finished

Step 7 completed the delegated acceptance proof for idea 157 after the safe
Step 6 consumer switches. The proof covered the backend BIR, prepared lookup,
and AArch64 backend call-boundary tests selected by the supervisor and passed
without implementation changes.

## Suggested Next

Supervisor should review the completed acceptance proof and decide whether to
route idea 157 to reviewer or plan-owner lifecycle handling.

## Watchouts

- Remaining prepared publication-routing source-selection reads are
  ABI/layout-bound and intentionally not switched under this source idea.
- Prepared call plans still own call instruction identity, argument index
  iteration, prepared value-home/register authority, recursive operand
  fallback, ABI placement, aggregate transport, scratch/final lowering, and
  target-specific emission.
- The Step 6 BIR source-producer switch converts only the semantic producer
  fact into local AArch64 materialization payload at the scalar call-argument
  boundary; do not treat it as BIR ownership of ABI placement or prepared
  numeric ids.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend acceptance
run: 179 backend tests passed, 0 failed.
