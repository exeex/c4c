Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for AArch64 prepared branch record error
naming: `prepared_branch_record_error_name` now delegates through an internal
lookup helper backed by an explicit target-local spelling table. All previous
public spellings were preserved, and missing/unknown enum values still return
`unknown`.

## Suggested Next

Continue Step 2 with a review packet over the remaining instruction record
naming and status helpers in `instruction.cpp`; if no direct public spelling
switches remain, either contract the next local status helper surface or return
a review note recommending supervisor transition toward Step 3.

## Watchouts

The branch spelling table intentionally mirrors the previous switch arms
one-for-one, including compare facts, unsupported compare predicate, and compare
value home entries. This packet only changed the local naming helper shape; no
diagnostics, tests, branch/comparison implementation files, or downstream
instruction selection behavior were changed.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_prepared_branch_records_test backend_aarch64_compare_branch_candidate_records_test backend_aarch64_target_instruction_records_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(prepared_branch_records|compare_branch_candidate_records|target_instruction_records)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
