# Call-Bundle And Multi-Function Prepared-Module Consumption

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Teach the x86 prepared emitter to consume cross-function module structure and
prepared call bundles as normal backend behavior instead of rejecting anything
beyond a single-function minimal route.

## Owned Failure Families

This idea owns these current diagnostic families:

- `error: x86 backend emitter only supports a single-function prepared module ...`
- `error: x86 backend emitter requires the authoritative prepared call-bundle ...`

Current sizes from the 2026-04-20 backend run:

- single-function prepared-module restriction: 18 failures
- prepared call-bundle requirement: 5 failures

## Current Known Failed Cases It Owns

Single-function prepared-module restriction cases:

- `c_testsuite_x86_backend_src_00077_c`
- `c_testsuite_x86_backend_src_00078_c`
- `c_testsuite_x86_backend_src_00083_c`
- `c_testsuite_x86_backend_src_00084_c`
- `c_testsuite_x86_backend_src_00087_c`
- `c_testsuite_x86_backend_src_00100_c`
- `c_testsuite_x86_backend_src_00140_c`
- `c_testsuite_x86_backend_src_00162_c`
- `c_testsuite_x86_backend_src_00168_c`
- `c_testsuite_x86_backend_src_00170_c`
- `c_testsuite_x86_backend_src_00189_c`
- `c_testsuite_x86_backend_src_00190_c`
- `c_testsuite_x86_backend_src_00196_c`
- `c_testsuite_x86_backend_src_00197_c`
- `c_testsuite_x86_backend_src_00198_c`
- `c_testsuite_x86_backend_src_00199_c`
- `c_testsuite_x86_backend_src_00200_c`
- `c_testsuite_x86_backend_src_00210_c`
- `c_testsuite_x86_backend_src_00219_c`

Prepared call-bundle failures:

- `c_testsuite_x86_backend_src_00021_c`
- `c_testsuite_x86_backend_src_00116_c`
- `c_testsuite_x86_backend_src_00121_c`
- `c_testsuite_x86_backend_src_00165_c`
- `c_testsuite_x86_backend_src_00177_c`

## Scope Notes

Expected repair themes include:

- multi-function prepared-module traversal
- entry-point and non-entry-point function emission under one module contract
- call-bundle consumption for argument setup, call clobber obligations, and
  result handoff

## Boundaries

This idea does not own:

- upstream semantic-lowering failures before module emission
- CFG handoff misses for short-circuit or guard-chain families
- generic scalar expression selection unrelated to calls or cross-function
  layout
- runtime correctness bugs after the call path already executes

## Dependencies

This idea depends on enough semantic lowering and value-location/addressing
facts being present for function-level handoff to be authoritative.

## Completion Signal

This idea is complete when the backend no longer rejects the owned cases due to
single-function-only or missing prepared call-bundle restrictions.
