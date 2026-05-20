Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Relocalize The Current Byval Lane First Bad Fact

# Current Packet

## Just Finished

Lifecycle switch complete. The umbrella backend regex inventory plan is
deactivated, and the active runbook now targets the existing focused owner
`ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`.

The switch is based on fresh `00204` evidence from the umbrella classification:
`fa_s1(s1)` currently reaches the call boundary with the prepared byval
temporary address in `x0` instead of the one-byte payload packed into `w0`.

## Suggested Next

Execute Step 1 from `plan.md`: relocalize the current byval lane first bad
fact against fresh generated AArch64 and prepared records for
`c_testsuite_aarch64_backend_src_00204_c`, then record the concrete source
bytes, prepared storage, ABI lane, and emitted callsite fact here before code
edits.

## Watchouts

- Preserve prior idea 328 repairs for rounded byval placement, coherent
  register-to-stack transition, and partial upper-lane `%9s` publication.
- Do not implement a named `00204` / `fa_s1` shortcut.
- Do not reopen HFA/floating, stdarg cursor, MOVI, fixed-formal, or
  local/value-home owners without a new first-bad-fact record.

## Proof

Lifecycle switch only; no code validation required. The umbrella classification
proof remains the existing `test_after.log` from the fresh backend-regex run.
