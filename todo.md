Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Preservation, Clobber, And Boundary Effects

# Current Packet

## Just Finished

Completed `plan.md` Step 3 narrow clobber-derivation cleanup: extracted the
caller-saved clobber span append/deduplication logic from
`build_call_clobber_set(...)` into file-local
`append_call_clobbered_register_spans(...)`.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `todo.md`

Extraction result:
- Added file-local `append_call_clobbered_register_spans(...)` with explicit
  clobber vector, target profile, register class, and contiguous-width inputs.
- Moved the existing caller-saved span scan, duplicate check, reserved-scratch
  placement conversion, and `PreparedClobberedRegister` append into that
  helper.
- Kept `build_call_clobber_set(...)` responsible for selecting the base
  register classes and widened register groups from regalloc values.
- Preserved target-profile ownership, caller-saved span selection, duplicate
  ordering, reserved-scratch placement mapping, and null-regalloc behavior.
- Audited preservation and boundary-effect helpers; they already have explicit
  file-local helper boundaries, so this packet did not move save/restore or
  boundary-effect construction.
- Left `calls.hpp`, `call_plans.hpp`, tests, `plan.md`, and the source idea
  untouched.

## Suggested Next

Proceed to `plan.md` Step 4 with a focused review of memory-return and formal
publication boundaries. Start by auditing whether memory-return result
construction has an equally clear file-local boundary after the argument and
clobber extractions; only extract it if inputs and outputs stay explicit.

## Watchouts

- Keep this slice behavior-preserving.
- Do not change ABI policy, argument/result placement, clobbering,
  preservation, indirect-callee handling, memory-return behavior, formal
  publication behavior, or prepared dump meaning.
- Keep `calls.hpp` as the aggregate public contract unless usage proves a
  smaller independently consumed boundary.
- Clobber derivation still depends on `caller_saved_register_spans(...)` and
  `TargetProfile`; keep target policy there instead of introducing local ABI
  rules.
- Preservation construction is still owned by `build_call_preserved_values(...)`;
  it was not split because the existing helper already owns save/restore
  route selection without leaking into `populate_call_plans(...)`.
- Boundary-effect construction is already grouped behind
  `plan_prepared_call_boundary_effects(...)` and its append helpers; this
  packet did not alter classification or effect ordering.

## Proof

Ran delegated proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed, 162/162 backend tests passed.

Also ran `git diff --check`; result: passed.

Proof log: `test_after.log`.
