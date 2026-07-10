Status: Active
Source Idea Path: ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Object-Runtime BinaryInst Boundary

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: refreshed focused object-runtime evidence for
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`.

The delegated focused proof passed. Current diagnostic boundary: none observed;
the object-runtime harness compiled the row to an RV64 ELF object, linked it,
and qemu returned the expected `7`.

Historical context for this row named
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering` at `function=main`, `block=entry`, `block_index=0`,
`instruction_index=7`, `instruction_kind=BinaryInst`, owner `i32 ...`. Current
prepared BIR still has the corresponding `BinaryInst` as
`%t2 = bir.add i32 %t0, %t1` after the call. Current object disassembly lowers
that fragment to `add t5,t3,t4` and stores/returns the result, so the prior
`BinaryInst` fragment is covered in the current build.

Prepared facts consumed by the object-runtime path are present:
`call_arg_source` for args 0 and 1 selects `frame_slot_address` for
`%lv.first` and `%lv.second`; the before-call move bundle publishes stack-slot
arguments to `a0` and `a1`; `call_arg_value_publication` ties `%lv.first` to
payload `%lv.left` and `%lv.second` to payload `%lv.right`; preservation facts
keep `%lv.left` and `%lv.right` in callee-saved registers and `%lv.first` /
`%lv.second` in stack slots across the call.

Owner classification: no current first owner is exposed. The former owner would
have been object-runtime/RV64 object lowering for `BinaryInst`, but current
evidence shows object emission, linking, relocation/object-writer behavior, and
qemu execution all succeed. Missing prepared fact publication is not supported
by the current prepared dump.

## Suggested Next

Run the Step 3 nearby regression proof for the focused object-runtime row plus
the byval route/runtime rows from idea 659. Since the Step 1 target already
passes and no Step 2 repair owner is observable, the supervisor may treat Step 2
as skipped/no-op for this runbook.

## Watchouts

- Treat byval route/runtime rows from idea 659 as regression surfaces, not the
  implementation target.
- Do not reopen prepared byval call-boundary behavior without fresh focused
  evidence from the target row.
- Do not absorb generic RISC-V object-emission work unless the refreshed row
  proves that layer is the first owner.
- Do not use testcase names, final object bytes, fixed registers, or source
  shape as the object-runtime instruction authority.
- The old `BinaryInst` diagnostic is stale evidence only; do not patch against
  it unless a fresh reproduction reappears.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload$') > test_after.log 2>&1
```

Result: exit `0`; build was up to date and
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload` passed.
Proof log: `test_after.log`.
