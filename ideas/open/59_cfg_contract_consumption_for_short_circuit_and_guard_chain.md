# CFG Contract Consumption For Short-Circuit And Guard-Chain

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make the x86 prepared emitter consume authoritative control-flow contracts for
short-circuit and guard-chain families as real backend capability, not only as
boundary-test coverage.

The current codebase has prepared handoff skeletons, but backend c-testsuite
still shows explicit misses in these CFG families.

## Owned Failure Families

This idea owns these current diagnostic families:

- `error: x86 backend emitter requires the authoritative prepared ...`
  where the missing contract is the short-circuit handoff
- `error: x86 backend emitter requires the authoritative prepared guard-chain ...`

Current sizes from the 2026-04-20 backend run:

- short-circuit handoff: 6 failures
- guard-chain handoff: 4 failures

## Current Known Failed Cases It Owns

Short-circuit handoff failures:

- `c_testsuite_x86_backend_src_00033_c`
- `c_testsuite_x86_backend_src_00042_c`
- `c_testsuite_x86_backend_src_00046_c`
- `c_testsuite_x86_backend_src_00109_c`
- `c_testsuite_x86_backend_src_00183_c`
- `c_testsuite_x86_backend_src_00212_c`

Guard-chain handoff failures:

- `c_testsuite_x86_backend_src_00038_c`
- `c_testsuite_x86_backend_src_00057_c`
- `c_testsuite_x86_backend_src_00092_c`
- `c_testsuite_x86_backend_src_00093_c`

## Scope Notes

This leaf owns the bridge from prepared CFG/control-flow facts to actual x86
emission for these families.

Expected repair themes include:

- materializing prepared short-circuit joins without falling back to local
  matcher reconstruction
- consuming prepared guard-chain structure generically
- validating nearby same-family cases so repair does not stop at the named
  tests alone

## Boundaries

This idea does not own:

- upstream semantic-lowering failures that never reach prepared x86
- call-bundle or multi-function prepared-module support
- broad scalar expression selection outside these CFG families
- runtime correctness bugs after a case already runs

## Dependencies

This idea depends on enough upstream lowering existing for the owned cases to
reach prepared emission.

## Completion Signal

This idea is complete when the owned short-circuit and guard-chain c-testsuite
families no longer fail due to missing authoritative prepared CFG handoff.
