Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The Call Move And Preservation Boundary

# Current Packet

## Just Finished

Step 5 closure review rejected closing
`ideas/open/02_aarch64_calls_emission_consolidation.md` after the completed
call printing/effect-publication checkpoint.

Reason: the source idea acceptance criteria are not yet satisfied. The
printing checkpoint narrowed `calls_printing.cpp` to machine-node construction,
but the AArch64 calls family still has a broad exported `calls.hpp` surface and
large target-local files, especially `calls_moves.cpp`,
`calls_preservation.cpp`, and `calls_dispatch_bridge.cpp`, that must be checked
for duplicate prepared call-plan, move, preservation, and argument-source
decision logic.

Close gate note: `test_after.log` was not present during this lifecycle review,
so the regression guard could not be accepted for closure. Closure was rejected
independently because durable source-idea work remains.

## Suggested Next

Execute Step 1 from `plan.md`: map the call move and preservation boundary.

Start by classifying exported declarations in `calls.hpp` for:

- before-call moves
- after-call moves
- before-return moves
- value moves
- prior preserved-value lookup
- preservation republication and population
- argument-source construction used by call-boundary moves
- byval aggregate move helpers that feed call arguments

Record in this file which helper family is the next coherent code-changing
target, any missing shared prepared-fact blocker, and the focused backend proof
command.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into whole dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific register, memory, frame-slot, byval lane, immediate,
  constant-materialization, and assembly spelling in AArch64 code.
- Keep source-idea progress tied to deleted duplication, moved ownership, or a
  sharper emission-only boundary.
- Do not revisit completed call printing/effect-publication routes unless this
  mapping proves one still owns duplicate prepared-fact authority.
- If the mapping proves no narrow calls-emission target remains, stop and send
  the lifecycle state back to plan-owner for closure or deactivation review.

## Proof

No new build or test proof was run for this lifecycle-only plan rewrite.

Most recent executor-reported broader proof from the completed checkpoint:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Recorded result in prior `todo.md`: backend subset passed 162/162 tests.
