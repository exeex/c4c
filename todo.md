Status: Active
Source Idea Path: ideas/open/666_rv64_callee_saved_gpr_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select The First Callee-Saved/Live-Value Boundary

# Current Packet

## Just Finished

Step 2 selected exactly one first implementation boundary from the Step 1
evidence: the shared RV64 object-route live-value consumption/order boundary.

Owned rows for the first repair packet:

- row 183,
  `backend_obj_runtime_rv64_prepared_object_data_static_local_storage`
- row 184,
  `backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage`
- row 219,
  `backend_obj_runtime_rv64_callee_saved_gpr_live_across_call`

The selected boundary is the object-route consumer that orders call-result
publication and callee-saved preserved-value restoration after a call. Step 1
showed prepared call facts already publish the call result from `a0` to
caller-saved `t0`, model `t0`/`a0` as clobbered, keep `s1`/`s2` out of the
call-clobber set, assign fixed callee-saved save slots, and emit matching
object-route prologue/epilogue save/restore. The stale runtime edge appears
when the RV64 object-route text moves a preserved callee-saved value back into
the caller-saved result register after `mv t0,a0` and before the fresh call
result is stored or consumed.

Positive contract to preserve in Step 3:

- rows 183 and 184 must continue to pass the CLI and codegen-route
  static-storage object-data checks, preserving the retired 663 evidence for
  symbols, layout, initializer payloads, and relocations.
- rows 183, 184, and 219 must continue to publish the call result from `a0`
  into the expected caller-saved result value and must continue to keep
  callee-saved values outside the call-clobber set.
- object-route prologue/epilogue callee-saved save/restore must remain intact;
  the repair must only change the consumer ordering that currently overwrites
  the fresh result with stale `s1`/`s2`.
- the focused runtime rows should pass only when the fresh call result is
  stored or consumed before any callee-saved preserved-value restore can
  overwrite its destination.

Fail-closed contract for Step 3:

- if call-result publication is missing, ambiguous, stale, or mismatched with
  the consumer destination, the route must fail closed with a precise
  diagnostic instead of guessing a fixed register.
- if a preserved callee-saved value and a fresh call result both claim the same
  object-route consumer register without a proven ordering fact, the route must
  fail closed rather than emitting a post-call stale restore.
- if clobber facts, callee-saved save slots, or save/restore emission are
  missing or contradict the consumer ordering, the route must fail closed at
  that proven owner instead of hiding the mismatch in final text.
- if rows 183 or 184 lose the static-storage object-data facts proven by the
  CLI/codegen tests, the packet is outside this selected boundary and must not
  be accepted as callee-saved/live-value progress.

## Suggested Next

Execute Step 3 by repairing the general RV64 object-route live-value
consumption/order rule for rows 183, 184, and 219. The implementation packet
should preserve call-result publication before any callee-saved preserved-value
restore can overwrite the result register, add or preserve fail-closed
diagnostics for missing, ambiguous, stale, or mismatched ordering facts, and
run the focused seven-test subset recorded below.

## Watchouts

- Keep byval payloads, pointer-local lowering, static-storage object-data
  publication/layout/initializer/relocation repair, packed local member
  offsets, CLI dump formatting, AArch64 dispatch, generic RISC-V object
  emission, and LLVM torture work outside this plan unless focused evidence
  proves the same first owner.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or unrelated backend families.
- Reject fixed-register or named-row shortcuts; repair one general
  callee-saved/live-value rule only after the first owner is proven.
- `c4c-objdump` rejects these relocatable objects because of unsupported text
  relocations and rejects linked binaries because they are not relocatable
  objects; use `riscv64-linux-gnu-objdump` for this focused linked/object
  disassembly evidence.
- The assembly-route text for row 219 lacks the stale post-call `mv t0,s2`
  overwrite that appears in object-route disassembly, so keep Step 2 focused
  on object-route consumption/order rather than broad call lowering.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_prepared_object_data_static_local_storage_obj|backend_cli_riscv64_prepared_object_data_static_local_initialized_storage_obj|backend_codegen_route_riscv64_prepared_object_data_static_local_storage|backend_codegen_route_riscv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_callee_saved_gpr_live_across_call)$' > test_after.log 2>&1`

Result: build passed, and the focused CTest reproduced the expected Step 2
boundary with 4 passed and 3 failed out of 7. Passing rows were the two CLI
object-data tests and the two codegen-route object-data tests. Failing rows
were exactly 183, 184, and 219 under RV64 object-runtime:
`row183 exit=224 expected=11`, `row184 exit=224 expected=24`, and
`row219 exit=1 expected=0`. Proof log: `test_after.log`.
