Status: Active
Source Idea Path: ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Direct-Call Argument-1 Completion And Return 829

# Current Packet

## Just Finished

Completed Step 4. The bounded direct-call argument-1 identity/type relation
from Step 2 still passes focused proof on the current tree.

## Suggested Next

Close 830 as capability-complete for the prerequisite. The completion record
should state that direct non-variadic specified call argument 1 now has native
structured identity/type relation proof sufficient for 829 to resume Step 2
and independently validate whether its selected parameter-definition tuple can
publish body-parameter authority.

## Watchouts

- Do not edit code for Step 3.
- Do not publish 829 body-parameter authority inside 830.
- Do not broaden to generic call arguments or other indices.

## Proof

Closed 831 accepted full-suite comparable baseline: 3038/3038 passed at commit
`4be820759` in `test_baseline.log`.

Passed Step 4 focused proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' ) > test_after.log 2>&1 && git diff --check`

Result: 1/1 tests passed; `git diff --check` passed.
