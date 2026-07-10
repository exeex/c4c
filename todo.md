Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` classified the
`builds_prepared_prior_preserved_arg_call_object()` failure without
implementation.

The malformed variadic saved-GPR publication admission case already passes:
the focused log no longer attributes the first generic rejection to
`rejects_malformed_variadic_saved_gpr_publications()`, and prior gdb evidence
showed that helper returns without calling `fail()`.

For `builds_prepared_prior_preserved_arg_call_object()`, the first failing
boundary is the object-level two-call relocation/layout contract:
`fail("expected two direct probe call relocations")`. The prepared fixture
models two direct extern `probe` calls; the call text helper has a separate
passing acceptance row for the same prior-preserved argument freshness route.
That leaves the likely owner as the RV64 prepared object call fragment and
relocation emission path in `src/backend/mir/riscv/codegen/object_emission.cpp`
(`fragment_for_prepared_call()` through `prepared_function_to_object_function()`
and `build_rv64_text_object_module()`), not admission, tests, expectations, or
baseline policy.

This remains in idea 664 scope because it is an RV64 prepared object-emission
internal boundary for ordinary prepared call objects. No implementation files,
tests, expectations, unsupported markers, allowlists, timeout/runtime policy,
or baseline accounting were edited.

## Suggested Next

Suggested next packet: delegate a c4c-executor repair slice owning
`src/backend/mir/riscv/codegen/object_emission.cpp` for the prior-preserved
argument direct-extern call object path. Start by proving why
`fragment_for_prepared_call()`/object-module construction does not satisfy the
two `R_RISCV_CALL_PLT` relocations and expected preservation
population/republication bytes for
`make_prepared_prior_preserved_arg_call_module()`.

## Watchouts

- `test_after.log` prints repeated generic messages without test names; use
  gdb/backtrace disambiguation before assigning ownership to any generic
  `expected prepared RV64 object path to reject` line.
- Evidence for this packet is in
  `build/agent_state/664_step4_prior_preserved_arg_probe/gdb_prior_preserved_arg_failure.txt`.
- Later failures remain in the focused log, including direct call relocations,
  byval/local-memory/sret rows, call-argument publication ordering, and
  diagnostic exactness. This packet did not absorb those.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, or baseline accounting for this slice.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still failed on later
`backend_riscv_object_emission` rows. `test_after.log` is the canonical proof
log. Additional gdb evidence for this packet shows
`builds_prepared_prior_preserved_arg_call_object()` reaches
`fail("expected two direct probe call relocations")`; the classification-only
slice is complete despite the known focused-test failures.
