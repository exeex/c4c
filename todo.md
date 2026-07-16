Status: Active
Source Idea Path: ideas/open/842_lir_restricted_first_class_value_unions.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire accepted universal boundary escape hatches

# Current Packet

## Just Finished

Completed plan.md Step 3 for the selected PHI value boundary.

Migrated the selected PHI verifier/printer consumer path so modeled PHI result
type queries read `LirPhiOp.boundary_value_type` as the semantic boundary
authority. PHI rendering now requires the same carrier and verifies it mirrors
`LirPhiOp.type_str` before using that field as compatibility/rendering text.

Focused coverage now proves stale or misleading `LirPhiOp.type_str` cannot
override the selected PHI boundary carrier for verification or rendering, while
valid PHI alternatives still verify and render with their parity text intact.

## Suggested Next

Execute plan.md Step 4 for the selected PHI boundary only: retire the accepted
PHI universal escape hatch only if every selected PHI consumer has migrated,
while leaving call, select, return, Raw-BIR, and unrelated families untouched.

## Watchouts

PHI rendering intentionally still emits `LirPhiOp.type_str` as parity text, but
the printer now validates `LirPhiOp.boundary_value_type` before rendering. Step
4 must not delete `LirPhiOp.type_str` unless the supervisor selects that exact
retirement and proves no remaining PHI consumer depends on it.

The Step 2 wrong-kind test covers runtime-text-only refs as the feasible local
surface for metadata-like/unbounded payload rejection. Partially parsed call
signature text and raw `args_str` payloads remain call-boundary concerns and
were not introduced into PHI.

## Proof

Step 3 proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_|^backend_'; } > test_after.log 2>&1`

Result: passed, 19/19 frontend/backend tests passing.

Additional proof: `git diff --check` passed.

Proof log: `test_after.log`.
