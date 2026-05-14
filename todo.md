Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Facts And Current Dispatch

# Current Packet

## Just Finished

Completed Step 1, Inspect Prepared Facts And Current Dispatch.

Prepared fact definitions inspected:

- `src/backend/prealloc/prealloc.hpp:951`: `PreparedMovePhase` includes
  `BeforeCall`, `AfterCall`, and `BeforeReturn`.
- `src/backend/prealloc/prealloc.hpp:842`: `PreparedMoveResolution` carries
  source/destination prepared value ids, destination ABI kind/storage/index,
  destination register/stack facts, cycle-temp/coalescing flags, block and
  instruction indexes, parallel-copy provenance, reason, and optional
  `PreparedRegisterPlacement`.
- `src/backend/prealloc/prealloc.hpp:866`: `PreparedAbiBinding` carries ABI
  destination kind/storage/index, register/stack facts, contiguous width,
  occupied registers, and optional placement.
- `src/backend/prealloc/prealloc.hpp:992`: `PreparedMoveBundle` is keyed by
  function, phase, block index, instruction index, optional edge provenance,
  and owns `moves` plus `abi_bindings`.
- `src/backend/prealloc/prealloc.hpp:181`: `PreparedFrameSlot` carries slot id,
  object id, function id, prepared offset, size, alignment, and fixed-location
  flag.
- `src/backend/prealloc/prealloc.hpp:313`: `PreparedSavedRegister` carries
  bank, physical register spelling, contiguous width, occupied registers,
  save index, and optional placement.
- `src/backend/prealloc/prealloc.hpp:322`: `PreparedFramePlanFunction` carries
  function id, frame size/alignment, saved callee registers, frame-slot order,
  `has_dynamic_stack`, and fixed-slot frame-pointer policy.
- `src/backend/prealloc/prealloc.hpp:1033`: `PreparedCallArgumentPlan` carries
  argument index, value bank, source encoding/value/base/literal/symbol/slot/
  stack/register facts, destination register or stack facts, contiguous width,
  occupied registers, and optional source/destination placements.
- `src/backend/prealloc/prealloc.hpp:1058`: `PreparedCallResultPlan` carries
  value bank, source/destination storage kinds, destination value id, source
  register/stack facts, destination register/slot/stack facts, width, occupied
  registers, and optional placements.
- `src/backend/prealloc/prealloc.hpp:1080`: `PreparedClobberedRegister` carries
  bank, register spelling, contiguous width, occupied registers, and optional
  placement.
- `src/backend/prealloc/prealloc.hpp:1107`: `PreparedCallPreservedValue`
  carries value/name ids, preservation route, callee-save save index, register
  or slot/stack facts, occupied registers, and optional register/spill
  placements.
- `src/backend/prealloc/prealloc.hpp:1144`: `PreparedIndirectCalleePlan`
  carries indirect callee value identity, encoding, bank, register/slot/stack/
  immediate/pointer-base facts, and optional placement.
- `src/backend/prealloc/prealloc.hpp:1158`: `PreparedMemoryReturnPlan` carries
  sret argument index, storage slot name, encoding, slot id, stack offset,
  size, and alignment.
- `src/backend/prealloc/prealloc.hpp:1168`: `PreparedCallPlan` is keyed by
  block and instruction index and carries wrapper kind, variadic FP register
  count, direct/indirect callee facts, memory return, arguments, result,
  preserved values, and clobbered registers.
- `src/backend/prealloc/prealloc.hpp:4258` and `:4292`: prepared lookup helpers
  already exist for frame plans and call plans by function id.

Current AArch64 call/frame dispatch stop points:

- `src/backend/mir/aarch64/codegen/dispatch.cpp:79` classifies
  `bir::CallInst` as `InstructionLoweringFamily::Call`.
- `src/backend/mir/aarch64/codegen/dispatch.cpp:164` still calls only
  `lower_scalar_instruction(...)` for every retained BIR instruction; failed
  calls fall through to `append_unsupported_instruction_diagnostic(...)` at
  `:173`, producing the existing unsupported call-family diagnostic.
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:224`
  currently asserts that a call instruction is still reported as unsupported.
  That test is the first expected update when call lowering is introduced.
- `src/backend/mir/aarch64/codegen/traversal.cpp:35` builds
  `FunctionLoweringContext` with prepared value locations, storage plan, and
  regalloc only. It does not yet thread `PreparedFramePlanFunction`,
  `PreparedDynamicStackPlanFunction`, or `PreparedCallPlansFunction` into the
  AArch64 lowering context.
- No AArch64 prologue/epilogue insertion point exists today: traversal lowers
  prepared blocks directly and dispatch lowers terminators, but frame setup and
  teardown records are not emitted around function/block lowering.

Selected machine-record and printer surfaces to edit next:

- `src/backend/mir/aarch64/module/module.hpp`: extend
  `FunctionLoweringContext` with prepared frame/call/dynamic-stack pointers
  before any lowering consumes those facts.
- `src/backend/mir/aarch64/codegen/traversal.cpp`: populate those pointers via
  the existing prepared lookup helpers.
- `src/backend/mir/aarch64/codegen/dispatch.cpp`: add a real call-family branch
  instead of routing calls through scalar lowering; keep missing prepared call
  facts fail-closed.
- `src/backend/mir/aarch64/codegen/instruction.hpp`: current
  `CallInstructionRecord` has only direct/indirect callee operands,
  argument/result operands, calling convention, and boolean flags. It needs
  structured prepared provenance before it can carry call-boundary authority:
  source `PreparedCallPlan`, wrapper kind, memory-return, preserved/clobber
  facts, and possibly selected call opcode/mnemonic fields.
- `src/backend/mir/aarch64/codegen/instruction.cpp`: `make_call_instruction`
  already creates a selected call node and call side effect, but it does not
  set function/block/instruction identity, selected call opcode, prepared
  clobbers, memory-return effects, or call-plan provenance.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`: printable families
  are currently spill/reload, branch, memory store, scalar add/sub, and return.
  There is no `print_call(...)`, no `bl`/`blr` mnemonic, and no frame
  prologue/epilogue printer path.
- Frame machine-node records are not present in code yet. The docs name
  `FrameRecord`, `FrameSlotRecord`, `CalleeSaveRecord`, `DynamicStackRecord`,
  `MoveRecord`, and `AbiBindingRecord`, but `rg` finds them only in contract
  markdown, not as current C++ records. Do not pretend those carriers exist in
  implementation code.

Missing/deferred facts that must not be invented target-locally:

- Do not choose argument/result registers, stack-argument placement, outgoing
  call area size, sret storage, caller/callee preservation, or scratch
  registers in AArch64 lowering; consume `PreparedCallPlan`,
  `PreparedMoveBundle`, `PreparedAbiBinding`, storage/regalloc facts, and frame
  plans.
- Do not borrow `x16`/`x17` as generic indirect-call scratch. The AAPCS64
  contract says those registers need explicit prepared/linker/veneer authority.
- Do not synthesize full variadic function-entry `va_list` or register-save
  area behavior. The present call-boundary fact is
  `PreparedCallPlan::variadic_fpr_arg_register_count`.
- Do not invent frame layout, callee-save slots, dynamic-stack anchoring, or
  frame-pointer policy. Use `PreparedFramePlanFunction`,
  `PreparedStackLayout`, and `PreparedDynamicStackPlan`.
- If a first implementation packet needs concrete code records for frame
  setup/teardown or target-MIR call snapshots beyond current
  `CallInstructionRecord`, add explicit structured records first; do not encode
  the missing facts as rendered assembly strings.

## Suggested Next

Start Step 2 by adding the minimum structured C++ record fields needed for a
direct fixed-arity call node and its prepared provenance, then wire dispatch to
consume an existing `PreparedCallPlan` for one register/immediate direct-call
case. Keep frame/prologue records separate unless the packet explicitly owns
them.

## Watchouts

- Do not reconstruct call ABI classification, frame layout, outgoing stack
  areas, callee-save slots, sret storage, or scratch policy inside AArch64
  codegen.
- Do not implement full variadic function-entry `va_list` behavior in this
  route.
- Reject named-case call/frame shortcuts, weakened expectations, or text-only
  printer patches as progress.
- The contract markdown mentions richer `module::CallRecord`/`FrameRecord`
  surfaces, but the current C++ implementation does not define those records.
  Treat that as an implementation gap, not permission to infer facts locally.

## Proof

Inspection-only packet; no build/test run and no `test_after.log` created.

First narrow proof subset recommended for the next implementation packet:

- Build:
  `cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_frame_stack_call_contract_test -j2`
- Test:
  `ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_frame_stack_call_contract)$'`

Rationale: `backend_prepare_frame_stack_call_contract` guards the prepared
call/frame facts, `backend_aarch64_instruction_dispatch` owns the current call
stop point, `backend_aarch64_target_instruction_records` owns selected call
record shape, and `backend_aarch64_machine_printer` owns printable selected
node output.
