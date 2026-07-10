Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` refreshed the delegated proof and rechecked the classified
malformed variadic saved-GPR publication admission owner.

Current source state already rejects duplicate
`rv64_incoming_variadic_gpr_publications`: gdb stopped at
`rejects_malformed_variadic_saved_gpr_publications()` and the function returned
without calling `fail()`. The RV64-local validator in
`src/backend/mir/riscv/codegen/object_emission.cpp` still owns the duplicate
source/destination diagnostic before prologue publication emission.

No implementation file needed an edit in this packet. The refreshed
`test_after.log` still contains a generic
`expected prepared RV64 object path to reject` line, but gdb identified that
line as `builds_prepared_prior_preserved_arg_call_object()`, not the delegated
malformed variadic saved-GPR publication case.

## Suggested Next

Suggested next packet: classify and repair the remaining generic
`expected prepared RV64 object path to reject` failure from
`builds_prepared_prior_preserved_arg_call_object()`, or choose the next
supervisor-owned focused failure if that prior-preserved-argument call route is
not the desired next slice.

## Watchouts

- `test_after.log` prints repeated generic messages without test names; use
  gdb/backtrace disambiguation before assigning ownership to any generic
  `expected prepared RV64 object path to reject` line.
- Later failures remain in the focused log, including direct call relocations,
  byval/local-memory/sret rows, call-argument publication ordering, and
  diagnostic exactness. This packet did not absorb those.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, or baseline accounting for this slice.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still failed on later
`backend_riscv_object_emission` rows. `test_after.log` is the canonical proof
log. Additional gdb checks showed the delegated
`rejects_malformed_variadic_saved_gpr_publications()` case returns without
calling `fail()`; the first generic rejection failure in the log is currently
`builds_prepared_prior_preserved_arg_call_object()`, outside this packet.
