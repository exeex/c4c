Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Review Memory-Return And Formal Publication Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 4 focused memory-return and publication-boundary
review: extracted call result construction from `populate_call_plans(...)` into
file-local `build_call_result_plan(...)` while leaving memory-return and formal
publication behavior unchanged.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `todo.md`

Extraction result:
- Added file-local `build_call_result_plan(...)` with explicit name tables,
  target profile, regalloc, value-location, after-call bundle, instruction
  index, and call inputs.
- Moved the existing call-result ABI source binding lookup, fallback source
  register placement, destination home lookup, destination value-id lookup, and
  destination register placement logic into that helper.
- Kept `build_memory_return_plan(...)` as the owned memory-return boundary; the
  audit found it already isolates sret storage-slot lookup and frame-slot
  selection from result-publication construction.
- Verified formal publication remains separate: it consumes function formals,
  formal ABI facts, value homes, and value-location lookups through
  `PreparedFormalPublicationInputs`, and does not need call-plan ABI policy or
  public contract movement for this slice.
- Left `calls.hpp`, `call_plans.hpp`, `formal_publications.cpp`,
  `formal_publications.hpp`, tests, `plan.md`, and the source idea untouched.

## Suggested Next

Proceed to `plan.md` Step 5 and align the prepared-printer call mirror only if
the earlier helper-family changes require naming or grouping updates. Preserve
printed fields and meaning; if the current dump mirror already follows the
aggregate call-plan data, record the no-code decision.

## Watchouts

- Keep this slice behavior-preserving.
- Do not change ABI policy, argument/result placement, clobbering,
  preservation, indirect-callee handling, memory-return behavior, formal
  publication behavior, or prepared dump meaning.
- Keep `calls.hpp` as the aggregate public contract unless usage proves a
  smaller independently consumed boundary.
- `build_memory_return_plan(...)` still owns only sret storage and frame-slot
  discovery; normal named result publication now stays behind
  `build_call_result_plan(...)`.
- Formal publication code deliberately remains independent from call plans; do
  not move it into call-plan helpers without a separate public-contract reason.
- Step 5 should inspect `prepared_printer/calls.cpp` against the new helper
  families, not rewrite labels for cosmetic reasons.

## Proof

Ran delegated proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed, 162/162 backend tests passed.

Also ran `git diff --check`; result: passed.

Proof log: `test_after.log`.
