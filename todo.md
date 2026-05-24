Status: Active
Source Idea Path: ideas/open/prealloc-call-boundary-ordering-republication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extend Neutral Call-Boundary Plan Data

# Current Packet

## Just Finished

Completed Step 2: `Extend Neutral Call-Boundary Plan Data`.

Added `PreparedCallBoundaryEffectPlan` plus endpoint/kind records and
`plan_prepared_call_boundary_effects` in prealloc. The helper builds ordered
records for before-call explicit prepared moves, preservation-home population,
after-call explicit prepared moves, and preservation republication intent from
`PreparedCallPlan` plus optional before/after `PreparedMoveBundle` inputs.

The records carry phase, call position, order index, move classification
status, destination class/storage/ABI index, neutral source and destination
storage/value facts, preservation route, and reason. They intentionally do not
carry AArch64 register views, machine instruction records, memory operand
strings, byval chunk sequences, scratch choices, or emitted operands.

Added `backend_call_boundary_effect_plan`, a focused MIR backend unit test that
constructs prepared call plans and move bundles directly and verifies explicit
move records, callee-saved/stack preservation population, republication intent,
and unavailable-move classification without using AArch64 codegen headers.

## Suggested Next

Execute Step 3 with a narrow AArch64 adapter slice that reads
`plan_prepared_call_boundary_effects` for one call-boundary path, preferably
explicit before-call move planning, while preserving the existing AArch64
instruction construction and register/operand policy.

## Watchouts

The new neutral records describe generic effect intent and storage identity,
not target operand legality. Keep AArch64 alias checks, register conversion,
memory operand spelling, scratch selection, byval chunking, indirect callee
materialization, and final instruction records target-local during Step 3.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `159/159` backend tests passed, `0` failed.

Proof log: `test_after.log`.
