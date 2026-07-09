Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Reclassify Rows After String-Label Pointer Authority

# Current Packet

## Just Finished

Completed Step 8: reran and reclassified the ten idea-630 representative rows
after Step 7 `StringConstantLabelPointer` authority.

Result summary:

- Total/passed/failed: 10 total, 3 passed, 7 failed.
- All ten representative rows now publish
  `layout_authority=string_constant_label_pointer` for the prior short-string
  pointer-materialization shape; none still shows the Step 6
  `layout_authority=unknown range_verdict=proven_out_of_bounds` blocker for
  that string-label pointer access.
- Rows consuming `StringConstantLabelPointer` and passing:
  `src/20010123-1.c`, `src/20030920-1.c`, `src/pr35800.c`.
- Rows consuming `StringConstantLabelPointer` but now owned by out-of-scope
  prepared move-bundle fan-in:
  `src/20011109-2.c`, `src/20021204-1.c`, `src/920429-1.c`,
  `src/930429-1.c`, `src/pr34415.c`, `src/ptr-arith-1.c`.
- Row consuming `StringConstantLabelPointer` but now failing after object/link
  success with `[RV64_BACKEND_RUNTIME_MISMATCH]`/c4c segfault:
  `src/20000722-1.c`; classify as out-of-scope runtime/object correctness, not
  a string-constant local-memory admission failure.
- No representative row remains in-scope for idea 630 based on the refreshed
  owner diagnostics. No `LoadGlobalInst` string-label pointer consumer gap
  appeared in this scan.

Row evidence:

| Row | Result | Current owner bucket |
| --- | --- | --- |
| `src/20000722-1.c` | fail | out-of-scope runtime/object correctness |
| `src/20010123-1.c` | pass | consumed `StringConstantLabelPointer`; idea-630 success |
| `src/20011109-2.c` | fail | out-of-scope prepared move-bundle fan-in |
| `src/20021204-1.c` | fail | out-of-scope prepared move-bundle fan-in |
| `src/20030920-1.c` | pass | consumed `StringConstantLabelPointer`; idea-630 success |
| `src/920429-1.c` | fail | out-of-scope prepared move-bundle fan-in |
| `src/930429-1.c` | fail | out-of-scope prepared move-bundle fan-in |
| `src/pr34415.c` | fail | out-of-scope prepared move-bundle fan-in |
| `src/pr35800.c` | pass | consumed `StringConstantLabelPointer`; idea-630 success |
| `src/ptr-arith-1.c` | fail | out-of-scope prepared move-bundle fan-in |

Detailed classification is saved in
`build/agent_state/630_step8_classification.md`.

## Suggested Next

Ask the plan owner to close or split the active lifecycle: Step 8 evidence
shows idea 630 is close-ready from the representative string-label pointer
authority surface, while remaining failures belong to separate owner families.

## Watchouts

Do not count the six prepared move-bundle fan-in failures or the
`src/20000722-1.c` runtime segfault as idea-630 completion work. They moved
past the string-label pointer authority blocker and should be split or routed
to their own owner ideas. The Step 8 scan did not expose a direct-global,
aggregate/block-entry stack-home, pointer-value byte access, select-carrier,
`LoadGlobalInst`, or string-byte/trailing-NUL extent authority row that remains
in-scope for idea 630.

## Proof

Ran the delegated proof/classification command exactly:

`cmake --build --preset default && ALLOWLIST=build/agent_state/630_step1_string_constant.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/630_step8_string_constant.log 2>&1`

Result: exited 1 because 7 of 10 representative rows still fail outside the
idea-630 authority bucket. The command produced
`build/agent_state/630_step8_string_constant.log`; summary rows are in
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`; focused prepared
authority extracts are in
`build/agent_state/630_step8_string_label_pointer_rows.tsv` and
`build/agent_state/630_step8_prepared_access_extract.txt`.
