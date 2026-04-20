# Scalar Expression And Terminator Selection For X86 Backend

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Replace the current minimal matcher-shaped x86 scalar emitter with generic
selection over prepared contracts for ordinary scalar expressions and
terminators.

This idea exists because a large share of current backend failures is not
"missing handoff field" but "x86 emitter still only supports a tiny special
case".

## Owned Failure Families

This idea owns these current diagnostic families:

- `error: x86 backend emitter only supports a minimal single-block i32 return ...`
- `error: x86 backend emitter only supports direct immediate i32 returns, ...`

Current sizes from the 2026-04-20 backend run:

- minimal single-block i32 return restriction: 34 failures
- direct-immediate-only scalar return restriction: 30 failures

## Current Known Failed Cases It Owns

Minimal single-block terminator / branch restriction examples:

- `c_testsuite_x86_backend_src_00007_c`
- `c_testsuite_x86_backend_src_00008_c`
- `c_testsuite_x86_backend_src_00034_c`
- `c_testsuite_x86_backend_src_00035_c`
- `c_testsuite_x86_backend_src_00036_c`
- `c_testsuite_x86_backend_src_00041_c`
- `c_testsuite_x86_backend_src_00058_c`
- `c_testsuite_x86_backend_src_00066_c`
- `c_testsuite_x86_backend_src_00081_c`
- `c_testsuite_x86_backend_src_00082_c`
- `c_testsuite_x86_backend_src_00101_c`
- `c_testsuite_x86_backend_src_00102_c`

Direct-immediate-only scalar return restriction examples:

- `c_testsuite_x86_backend_src_00019_c`
- `c_testsuite_x86_backend_src_00023_c`
- `c_testsuite_x86_backend_src_00024_c`
- `c_testsuite_x86_backend_src_00025_c`
- `c_testsuite_x86_backend_src_00026_c`
- `c_testsuite_x86_backend_src_00056_c`
- `c_testsuite_x86_backend_src_00062_c`
- `c_testsuite_x86_backend_src_00067_c`
- `c_testsuite_x86_backend_src_00068_c`
- `c_testsuite_x86_backend_src_00069_c`
- `c_testsuite_x86_backend_src_00070_c`
- `c_testsuite_x86_backend_src_00107_c`

This idea also owns any other current backend failures with those same
diagnostic families.

## Scope Notes

Expected repair themes include:

- generic scalar value selection instead of direct-immediate-only returns
- ordinary compare / branch / return terminator emission over prepared facts
- removing single-block assumptions where prepared CFG/value data is already
  authoritative

## Boundaries

This idea does not own:

- upstream semantic-lowering failures before x86 prepared emission
- missing short-circuit or guard-chain handoff contracts
- call-bundle support
- multi-function prepared-module support
- runtime correctness bugs after successful execution starts

## Dependencies

This idea should build on upstream semantic coverage plus the relevant prepared
CFG and value-home facts.

## Completion Signal

This idea is complete when the current scalar-expression and minimal-terminator
diagnostic families are no longer the reason those owned cases fail.
