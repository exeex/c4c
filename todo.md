Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` refreshed the focused proof and classified the new first
remaining `backend_riscv_object_emission` failure after the Step 3 post-helper
traversal repair.

The selected `loads_rv64_va_start_published_word_after_helper` failure remains
resolved: fresh `test_after.log` no longer contains
`expected prepared va_start load object text layout`.

New first remaining failure: `expected prepared RV64 object path to reject`,
from `rejects_malformed_variadic_saved_gpr_publications()`. The first mutated
case duplicates a prepared RV64 incoming variadic GPR publication and expects
the RV64-local diagnostic
`unsupported_function_admission: RV64 variadic entry incoming GPR publications must not contain duplicate sources or destinations`,
but the prepared object route currently succeeds.

Likely owner: RV64 object-emission function-admission validation for
`rv64_incoming_variadic_gpr_publications`, before object construction publishes
a module. This is still in idea 664 scope as an object-emission infrastructure
fail-closed admission contract, not a relocation, final object byte, baseline,
or unrelated runtime semantic repair.

## Suggested Next

Suggested next packet: repair the RV64 variadic incoming GPR publication
admission/fail-closed path so malformed duplicate source/destination
publications reject with the existing RV64-local diagnostic before prepared
object module construction succeeds. Keep the packet limited to that admission
contract and its existing fail-closed malformed-publication variants.

## Watchouts

- The classification did not implement a fix and did not touch `plan.md`,
  `ideas/open/664_riscv_object_emission_internal_probe.md`, implementation
  files, tests, expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or unrelated backend families.
- Later failures remain in the focused log, including direct call relocations,
  byval/local-memory/sret rows, call-argument publication ordering, and
  diagnostic exactness. Do not absorb those into the next packet.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, or baseline accounting for this slice.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still failed on later
`backend_riscv_object_emission` rows. `test_after.log` is the canonical proof
log. It no longer contains the selected va_start post-helper load failure; its
first remaining failure is now the malformed variadic saved-GPR publication
admission case described above.
