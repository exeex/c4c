Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: COMPLETE
Current Step Title: Runbook exhausted; awaiting plan-owner close/repair decision

# Current Packet

## Just Finished

Implemented Step 7.52 for the accepted 867 handoff receiver row. Raw BIR now
receives selected direct-local `LirVaStartOp` destination `va_list` authority
as a typed `VaStartAuthority` node/spec/accessor, imports only the structured
`ap_authority` tuple when `ap_ptr.value_id()` matches the pointer definition,
and verifies the result against the matching live current-function alloca
authority.

Focused receiver coverage was added to
`backend_lir_to_bir_interface_test.cpp` for the positive receipt and malformed
selected/unselected boundaries: missing authority, non-SSA or mismatched
`ap_ptr`, mismatched pointer definition, foreign owner, object mismatch,
pointer/pointee type mismatch, dead authority, and authority on unselected
`va_start`.

## Suggested Next

Route the exhausted runbook to plan-owner for an explicit close, repair,
replace, or conclude decision for
`ideas/open/734_lir_to_new_bir_container_completeness.md`.

## Watchouts

Do not repeat accepted idea 734 Steps 1 through 7.51. Do not edit LIR producer
authority or receive `va_end`, `va_copy`, `va_arg`, memcpy, memset, local/VLA,
prepared-BIR helper-home, target backend, aggregate/vector, parameter,
module/type/global, CFG/PHI, instruction/terminator, inline-assembly, or
generic residual rows. Never recover authority from spelling, printer output,
LLVM text, intrinsic names, rendered names, compatibility mirrors, or testcase
identity.

## Proof

Completed proof:

```sh
{ cmake --build build && ctest --test-dir build -R '^backend_lir_to_bir_interface$|^backend_lir_selected_pointer_authority$' --output-on-failure; } > test_after.log 2>&1
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, 2/2 focused tests before and after, no regression delta.
