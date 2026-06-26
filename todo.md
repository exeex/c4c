Status: Active
Source Idea Path: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Choose The Narrow Sret Route

# Current Packet

## Just Finished

Step 2 pinned the `unsupported_local_memory_access` boundary from Step 1 and
classified it as a prerequisite inside the idea 387 same-module sret route.

The exact failing prepared memory access is in callee `f`, not caller `main`:
`block_4 inst_index=2`, the terminal
`bir.store_local %lv.X.0, i32 f.ret.sret.copy.0, addr %ret.sret`. Its prepared
memory row is `base=pointer_value`, `stored=f.ret.sret.copy.0`,
`pointer=%ret.sret`, `offset=0`, `size=4`, `align=4`,
`base_plus_offset=yes`.

`%ret.sret` is stack-homed, not GPR-homed:
`home %ret.sret value_id=0 kind=stack_slot slot_id=0 offset=0`. The stack
object is the sret parameter object:
`object #4 func=f name=%ret.sret source_kind=sret_param type=ptr size=4 align=4
address_exposed=yes requires_home_slot=yes permanent_home_slot=yes`.

The rejecting object-emission guard set is in
`fragment_for_prepared_store_local` / `diagnose_unsupported_prepared_instruction_fragment`:

- `prepared_frame_slot_absolute_offset` rejects because the access base is
  `pointer_value`, not `frame_slot`.
- `prepared_byval_stack_slot_pointer_access_offset` only admits stack-homed
  pointer values for stack objects with `source_kind == "byval_param"`;
  `%ret.sret` is `source_kind=sret_param`.
- `prepared_pointer_value_base_offset` rejects because `%ret.sret` is not in a
  GPR home.

Route decision: do not split. This is directly required for idea 387 because
the callee-side sret return store must write through the incoming sret pointer
before the same-module call can return the memory result to the caller object.
The caller-side call plan still carries the later sret authority:
`memory_return=%t8`, `memory_encoding=frame_slot`, `sret_arg_index=0`,
`memory_slot=#7`, `memory_stack_offset=16`.

Detailed evidence:
`build/agent_state/387_step2_local_memory_route.log`.

## Suggested Next

Execute Step 3 from `plan.md`: add the narrow RV64 object-emission prerequisite
for stack-homed `sret_param` pointer-value local memory accesses. The repair
should load the sret pointer value from its stack home into a temporary GPR,
materialize the stored scalar into another GPR, and store through the loaded
pointer plus the prepared byte offset. After that, rerun the representative and
return to the existing same-module `memory_return=%t8` call-emission gate.

## Watchouts

- Keep the Step 3 guard narrow: default address space, nonvolatile,
  base-plus-offset pointer access, supported scalar size, valid alignment,
  signed 12-bit offset, stack-homed pointer value, and a matching stack object
  with `source_kind=sret_param`.
- Preserve existing `byval_param` behavior; do not broaden the byval helper by
  silently treating every stack-homed pointer as direct pointee storage.
- Do not delete or weaken the `memory_return` gate. It remains the next
  expected boundary after the callee-side sret pointer store is admitted.

## Proof

Proof for this packet: inspected existing Step 1 logs and implementation paths
only; no build or tests were run.

AST-backed navigation used `c4c-clang-tool-ccdb` on
`src/backend/mir/riscv/codegen/object_emission.cpp` to locate
`fragment_for_prepared_instruction`, its callees, and the offset helpers.

Logs inspected:
`build/agent_state/387_step1_analysis.log`,
`build/agent_state/387_step1_920908-1.prepared.log`, and
`build/agent_state/387_step1_920908-1.case.log`.

Result: route classified as local-memory prerequisite inside idea 387, followed
by the existing same-module sret `memory_return` gate.
