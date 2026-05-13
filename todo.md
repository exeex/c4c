Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Existing Allocation Surfaces

# Current Packet

## Just Finished

Step 1 of `plan.md` audited the allocation-sensitive AArch64 surfaces before
implementation edits.

Implemented surfaces that need allocation-boundary alignment:

- `src/backend/mir/aarch64/abi/abi.hpp` and
  `src/backend/mir/aarch64/abi/abi.cpp`: already define typed AArch64 register
  banks/views and caller/callee-save helper sets, including `x16`/`x17`
  indirect-call scratch, `x18` platform-reserved, `x29` frame pointer, `x30`
  link, `sp`, `x8` sret, and separate GPR vs FP/SIMD registers. Conflict:
  these helpers expose ABI register facts but do not yet classify finite pools
  into long-lived allocatable, argument/return, caller-saved temp, reserved MIR
  scratch, and special/forbidden roles.
- `src/backend/mir/aarch64/module/module.hpp` and
  `src/backend/mir/aarch64/module/module.cpp`: snapshot prepared value homes,
  regalloc assignments, storage plans, call plans, moves, ABI bindings,
  callee-save/clobber records, spill/reload ops, and frame slots. Conflict:
  target records still carry rendered `std::string_view` physical register
  names and concrete `stack_offset_bytes`; Step 3 should document or narrow
  them as prepared-result snapshots, not target-local allocation authority.
- `src/backend/mir/aarch64/codegen/records.hpp` and
  `src/backend/mir/aarch64/codegen/records.cpp`: target MIR records already
  distinguish `RegisterOperandRole::{PreparedAssignment, ValueHome,
  SpillAuthority, StoragePlan, CallAbi}` and convert prepared register strings
  to typed AArch64 `RegisterReference`. Conflict: scalar ALU/cast record
  builders currently require matching register value-home/storage facts and
  fail closed for stack/spill homes; memory/call/return records have no shared
  allocation-result carrier yet for virtual-register placeholders, reserved
  scratch use, or structured spill materialization plans.

Markdown artifacts that need allocation-boundary alignment:

- `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`: already requires
  prepared addressing, liveness, allocation, storage, call, frame, move,
  spill/reload, and ABI-binding facts. Step 4 should make the new allocation
  contract the named authority before target MIR and machine nodes.
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`: currently says
  `PreparedRegalloc`, value locations, storage plans, move bundles,
  spill/reload ops, frame plans, and call plans are present carriers, while
  target-local register/reference and move/spill/reload records remain design
  work. Step 4 should update any stale "missing" rows against the implemented
  module/record surfaces and keep true shared-carrier gaps explicit.
- `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`: should continue excluding
  obsolete/text-first routes and should mark allocation-sensitive legacy notes
  as design inputs only, not allocation authorities.
- `src/backend/mir/aarch64/codegen/records.md`: already warns that record
  construction must not recover operands from assembly text, allocate
  registers, choose spill code, or invent target-local storage; Step 4 should
  connect that warning to the allocation contract.
- `src/backend/mir/aarch64/codegen/alu.md`,
  `src/backend/mir/aarch64/codegen/cast_ops.md`,
  `src/backend/mir/aarch64/codegen/comparison.md`,
  `src/backend/mir/aarch64/codegen/memory.md`,
  `src/backend/mir/aarch64/codegen/calls.md`,
  `src/backend/mir/aarch64/codegen/returns.md`,
  `src/backend/mir/aarch64/codegen/prologue.md`,
  `src/backend/mir/aarch64/codegen/variadic.md`,
  `src/backend/mir/aarch64/codegen/asm_emitter.md`,
  `src/backend/mir/aarch64/codegen/inline_asm.md`,
  `src/backend/mir/aarch64/codegen/atomics.md`,
  `src/backend/mir/aarch64/codegen/f128.md`,
  `src/backend/mir/aarch64/codegen/float_ops.md`,
  `src/backend/mir/aarch64/codegen/i128_ops.md`, and
  `src/backend/mir/aarch64/codegen/intrinsics.md`: these legacy notes describe
  fixed accumulators (`x0`, `w0`, `x1`, `x2`, `x3`), fixed FP/SIMD carriers
  (`s0`, `d0`, `q0`, `v0`, `q1`), ad hoc scratch (`x9`, `x10`, `x16`, `x17`,
  `w12`), assigned callee-saved destination registers, stack offsets, outgoing
  argument spill areas, F128 helper scratch preservation, inline-asm scratch
  pools, and prologue `used_callee_saved` behavior. Step 4 should keep these as
  rebuild hazards and route long-lived homes, reserved scratch, spills,
  reloads, and call resources through the shared allocation result.
- `src/backend/mir/aarch64/codegen/emit.md`: flags the old entry route as
  direct BIR/LIR to text with fixed scratch registers; it should continue
  routing rebuilt lowering into structured target MIR instead of restoring text
  emission as an allocation authority.

Shared-carrier concerns found:

- Existing shared prepared carriers are present for value homes, regalloc
  assignments, storage plans, frame slots, saved/clobbered registers, calls,
  moves, ABI bindings, parallel copies, and spill/reload ops.
- Potential Step 5 gap to decide: the current AArch64-facing record layer does
  not expose a single allocation-result contract that names finite ABI pools,
  reserved MIR scratch pools, virtual-register placeholders, abstract spill-slot
  ids, and reload/store pseudo plans independently from rendered register names
  and concrete stack offsets.
- No Step 1 blocker was found that requires editing `ideas/open/` before the
  Step 2 markdown allocation contract.

## Suggested Next

Execute Step 2: add the AArch64 BIR/prepared-to-MIR allocation-boundary
contract under `src/backend/mir/aarch64/`, defining ABI register pools,
reserved MIR scratch policy, allocation-result shapes, spill-slot identity, and
portable x86/x86-64 correctness carriers without implementing an allocator.

## Watchouts

- Do not implement a full register allocator.
- Do not store AArch64 physical-register names, assembly text, or concrete
  stack offsets as core BIR semantic truth.
- Keep GPR and FPR/SIMD register classes separate.
- Keep reserved MIR scratch registers unavailable for long-lived BIR value
  homes.
- Create a separate `ideas/open/` gap idea if missing shared BIR/prepared
  carriers block the contract.
- Treat legacy markdown fixed-register conventions as rebuild hazards unless
  the new contract explicitly assigns them as ABI resources or reserved scratch.
- Module records currently expose prepared register names and stack offsets;
  Step 3 should either document those as snapshots or introduce narrow typed
  fields, not make module construction allocate registers.

## Proof

`git diff --check` passed. Proof output was preserved in `test_after.log`;
the file is empty because the command produced no diagnostics.
