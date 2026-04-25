Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Final Behavior-Preservation Proof

Code Review Reminder Handled: review/address-projection-step3-review.md found Step 3 helper reuse aligned and recommended continuing Step 3.
Test Baseline Reminder Handled: accepted full-suite test_baseline.log for commit f294c3a4 after 3071 passed, 0 failed.

# Current Packet

## Just Finished

Step 4 is complete for the delegated naming-normalization packet.

Renamed the ambiguous `AggregateByteOffsetProjection` offset fields within the
active consolidation surface:

- `child_byte_offset` is now `byte_offset_within_child`.
- `child_absolute_byte_offset` is now `child_start_byte_offset`.

The change is naming-only across `memory_helpers.hpp`, `addressing.cpp`, and
`local_gep.cpp`; behavior, diagnostics, BIR output policy, and expectations
were not changed.

## Suggested Next

Proceed to Step 5 Final Behavior-Preservation Proof.

Expected proof:

- Build `c4c_codegen`.
- Run relevant BIR/LIR-to-BIR GEP, aggregate, provenance, and memory tests.
- Escalate to a broader repo-native check if the supervisor wants milestone
  confidence beyond the accepted full-suite baseline for commit f294c3a4.
- Do not rewrite expectations.

## Watchouts

- This packet intentionally did not broaden into cosmetic renames beyond the
  delegated projection helper field/call-site surface.
- A repo-wide stale-name scan only found unrelated vendored LLVM reference
  matches under `ref/llvm-project/lldb/`.

## Proof

Ran the delegated proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.
