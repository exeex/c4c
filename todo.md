Status: Active
Source Idea Path: ideas/open/75_shared_aggregate_transport_plan_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Aggregate Transport Decisions

# Current Packet

## Just Finished

Step 1 of `plan.md` completed an inventory of aggregate transport decision
points before any implementation migration.

Before this packet, the plan only named the suspected missing authority. After
the inventory, the missing shared authority appears real:

- Stack-backed call-argument aggregate copies are planned in AArch64 lowering.
  `instruction.cpp` / `instruction.hpp` expose `aggregate_stack_copy_chunks`,
  load/store mnemonic helpers, and scratch selection. `calls.cpp` consumes
  those helpers in `make_aggregate_stack_copy_instruction` and
  `make_byval_register_lane_stack_publication_instruction`, deriving chunk
  widths, source/destination memory chunks, printable fallback to byte chunks,
  and scratch register use locally.
- Register-backed byval aggregate lane publication is only partially prepared.
  `prealloc/regalloc/call_moves.cpp` and `call_return_abi.cpp` identify
  AArch64 byval register-lane aggregate moves from ABI facts and publish
  `call_arg_byval_aggregate_register_lanes`, destination register names,
  occupied registers, and source storage encoding. They do not publish the
  reusable lane/chunk transport plan. AArch64 then re-derives lane width,
  printable chunk width, lane destination registers, scratch choice, and
  source memory chunks through `is_aggregate_register_lane_publication`,
  `aggregate_register_lane_printable_chunk`,
  `aggregate_register_lane_destination`, `aggregate_register_lane_scratch`,
  and `print_aggregate_register_lane_publication_lines`.
- Mixed or lane-sensitive aggregate transport has a model in variadic aggregate
  `va_arg`, but not a general call-argument transport authority.
  `prealloc/variadic_entry_plans.cpp` prepares source class, payload
  size/alignment, copy size, source slot/field offsets, lane count/size, lane
  destination homes, progression stride, and overflow stride. `variadic.cpp`
  consumes that prepared access plan, but still owns local aggregate
  copy-chunk, mnemonic, data-register, and destination-store helpers for
  lowering text.
- `memory.cpp` plus the i128/f128 carrier helpers are not the general answer.
  They use prepared scalar-carrier transport records and structured memory
  operands for special i128/f128 carrier movement. That proves the prepared
  transport-record pattern exists, but the authority is scalar-carrier
  specific rather than aggregate chunk/lane transport.

Decision classification:

- Prepared/shared today: byval aggregate ABI classification, destination
  register names and occupied-register facts, source storage encoding, and the
  richer variadic aggregate `va_arg` access plan.
- AArch64-local by design: final load/store mnemonic choice, AArch64 scratch
  register identity, memory-address printability checks, inline assembly text,
  target instruction record construction, and printer spelling.
- Missing shared authority: a reusable aggregate transport plan that names
  transport kind, source/destination homes, chunk boundaries, lane bindings,
  lane sizes/counts, source/destination byte offsets, printable/fallback chunk
  strategy, and scratch requirements for stack-backed, register-backed, and
  lane-sensitive aggregate paths.

## Suggested Next

Draft Step 2's prepared aggregate-transport contract in `todo.md` before code
changes. The contract should borrow the variadic aggregate `va_arg` model, but
generalize it for call-argument transport with fields for transport kind,
source/destination homes, copy size/alignment, chunks, lanes, offsets,
destination ABI registers or lane homes, and scratch requirements. It should
also state which AArch64 details remain target-local.

## Watchouts

- Do not implement AArch64 aggregate copy cleanup before naming the shared
  authority contract.
- Do not treat ordinary load/store opcode choice or address spelling as shared
  aggregate transport authority.
- Do not weaken aggregate testcase expectations or add named-case shortcuts.
- Keep general memory address cleanup out of this route.
- Do not present i128/f128 carrier transport as the aggregate solution; it is a
  useful prepared-record precedent, not the general aggregate chunk/lane
  authority.
- Avoid a helper-only rename. Step 2 must specify reusable facts that stop
  AArch64 consumers from re-deriving chunk/lane/home/scratch decisions.

## Proof

Read-only inventory plus `todo.md` update only; no build or test run required,
and no proof log was produced.
