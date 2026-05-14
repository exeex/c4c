Status: Active
Source Idea Path: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Reconcile ABI, Call, Frame, And Global Shards

# Current Packet

## Just Finished

Completed `plan.md` Step 4 ledger reconciliation for the ABI, call, frame, and
global shard group: `calls.md`, `prologue.md`, `variadic.md`, and `globals.md`.
Each entry separates current prepared/shared authority from real AArch64
codegen work and avoids reviving the archived markdown-era translation units.

Shard: `src/backend/mir/aarch64/codegen/calls.md`
Classification: real-missing-feature
Current Owner: prepared call authority in `src/backend/prealloc/prealloc.hpp`
and `src/backend/prealloc/prealloc.cpp`; partial AArch64 record vocabulary in
`src/backend/mir/aarch64/codegen/instruction.hpp` and
`src/backend/mir/aarch64/codegen/instruction.cpp`; no compiled AArch64 call
lowering or printer owner yet.
Review Result: The shard's old local ABI classifier and `x0`/`x1`/`q0`/`x17`
scratch conventions are stale authority. The valid current intent is broader:
selected AArch64 call machine nodes must consume prepared direct/indirect
callee identity, argument/result placement, memory-return, preserved-value,
clobber, `BeforeCall`/`AfterCall`, ABI-binding, and variadic call-boundary
facts. Current code has those prepared carriers, but AArch64 dispatch only
classifies `bir::CallInst` as `Call` and then reports it through the generic
unsupported-instruction path; `make_call_instruction` exists as record
vocabulary but no lowering path reaches it and the terminal printer has no call
case.
Proof or Evidence: `prealloc.hpp:1033` defines `PreparedCallArgumentPlan`;
`:1058` defines `PreparedCallResultPlan`; `:1107` defines call preserved-value
records; `:1122` defines wrapper kinds including direct extern variadic and
indirect; `:1168` defines `PreparedCallPlan`; `prealloc.cpp:1109` populates
call plans with wrapper kind, indirect callee, memory return, preserved values,
and clobbers; `dispatch.cpp:91` classifies `bir::CallInst` as `Call` but
`dispatch.cpp:170` only invokes `lower_scalar_instruction`; `alu.cpp:307`
returns no lowering for non-binary instructions; `instruction.cpp:1889`
defines call record construction; `machine_printer.cpp:365` through `:381`
prints spill/reload, branch, memory, scalar, and return only.
Follow-Up: `ideas/open/231_aarch64_call_frame_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/prologue.md`
Classification: real-missing-feature
Current Owner: prepared frame authority in `src/backend/prealloc/prealloc.hpp`,
`src/backend/prealloc/prealloc.cpp`, and
`src/backend/prealloc/stack_layout/coordinator.cpp`; partial frame-slot and
spill/reload record vocabulary in `src/backend/mir/aarch64/codegen/operands.cpp`,
`src/backend/mir/aarch64/codegen/instruction.hpp`, and
`src/backend/mir/aarch64/codegen/machine_printer.cpp`; no compiled AArch64
prologue/epilogue owner yet.
Review Result: The shard's old stack-space calculator, local register pool
selection, parameter pre-store optimization, and variadic save-area allocation
are not current AArch64 codegen authority. Current prepared layers own frame
size, alignment, frame-slot offsets, saved callee registers, dynamic-stack
state, and frame-pointer policy. The AArch64 backend can name frame-slot and
spill/reload records, but it does not yet lower prepared frame facts into
function-entry setup, callee-save save/restore nodes, epilogue teardown, or
parameter-home moves. Full variadic save-area layout remains a separate
prepared-carrier gap, not a prologue-local codegen shortcut.
Proof or Evidence: `prealloc.hpp:181` defines `PreparedFrameSlot`;
`:322` defines `PreparedFramePlanFunction` with frame size, alignment, saved
callee registers, frame-slot order, dynamic-stack, and frame-pointer flags;
`prealloc.cpp:820` populates the frame plan; `prealloc.cpp:845` takes function
frame size from prepared addressing; `prealloc.cpp:913` records saved
callee-register facts; `stack_layout/coordinator.cpp:776` through `:789`
builds per-function prepared addressing frame metadata; `machine_printer.cpp:89`
prints only already-selected spill/reload frame-slot nodes and no prologue or
epilogue frame sequence.
Follow-Up: `ideas/open/231_aarch64_call_frame_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/variadic.md`
Classification: real-missing-feature
Current Owner: minimal variadic call-boundary facts in
`src/backend/prealloc/prealloc.hpp` and `src/backend/prealloc/prealloc.cpp`;
none for full AAPCS64 variadic function-entry, `va_list`, `va_start`,
`va_arg`, or `va_copy` carriers.
Review Result: The accepted current implementation surface is narrower than
the archived shard. Prepared call plans preserve direct extern variadic wrapper
kind and the floating-point argument-register count for variadic calls, so the
call-boundary minimum is already represented outside AArch64 markdown. The
full `va_list` layout, register-save areas, negative GP/FP offsets, named
argument counts, stack overflow-area policy, aggregate `va_arg`, F128 helper
handling, and `va_copy` semantics are real missing mechanisms, but they are
prepared/shared carrier gaps first. AArch64 codegen must not synthesize those
facts from prologue text or markdown scratch-register conventions.
Proof or Evidence: `prealloc.hpp:1122` includes
`PreparedCallWrapperKind::DirectExternVariadic`; `prealloc.hpp:1172` stores
`PreparedCallPlan::variadic_fpr_arg_register_count`; `prealloc.cpp:295`
classifies direct extern variadic calls; `prealloc.cpp:319` computes variadic
FPR argument counts from call ABI; `prealloc.cpp:1112` stores both facts in
each prepared call plan. No prepared structs found for AAPCS64 `va_list`
fields, variadic register-save areas, `va_start`, `va_arg`, or `va_copy`.
Follow-Up: `ideas/open/232_aarch64_variadic_function_entry_carriers.md`.

Shard: `src/backend/mir/aarch64/codegen/globals.md`
Classification: real-missing-feature
Current Owner: BIR global/string memory identity in `src/backend/bir/bir.hpp`;
prepared symbol-backed memory access facts in
`src/backend/prealloc/stack_layout/coordinator.cpp`; partial AArch64 memory
record validation in `src/backend/mir/aarch64/codegen/instruction.cpp`; no
compiled AArch64 global/label/TLS address-materialization owner yet.
Review Result: Current owners already represent memory accesses whose base is a
global symbol or string constant, but the shard describes address-producing
operations: direct global address, label address, GOT-indirect global address,
and TLS address materialization through `tpidr_el0`. Treating global load/store
support as enough would conflate symbol-backed memory access with materialized
addresses. BIR has default/FS/GS/TLS address-space vocabulary and label base
identity, but prepared addressing currently maps only global symbols and string
constants into direct symbol-backed prepared addresses; AArch64 printer memory
output only handles frame slots and pointer-value register bases. Address-kind
policy for direct/GOT/TLS materialization is therefore a real follow-up, with
some carrier work and some AArch64 machine-node/printer work.
Proof or Evidence: `bir.hpp:94` defines `AddressSpace::Tls`; `bir.hpp:270`
defines `MemoryAddress::BaseKind::GlobalSymbol`, `Label`, and `StringConstant`;
`bir.hpp:412` and `:425` define load/store global instructions with semantic
`LinkNameId`; `stack_layout/coordinator.cpp:261` builds direct symbol-backed
prepared addresses; `stack_layout/coordinator.cpp:307` accepts only global
symbol and string constant bases for that path; `instruction.cpp:874` through
`:909` validates global symbol identity for memory operands; `instruction.cpp:1049`
maps prepared global-symbol addresses to `MemoryBaseKind::Symbol`; `machine_printer.cpp:38`
prints memory addresses only for frame slots and pointer values with base
registers.
Follow-Up: `ideas/open/233_aarch64_global_address_materialization.md`.

## Suggested Next

Execute `plan.md` Step 5 by reconciling only the scalar, memory, float, and
wide-operation shard group: `memory.md`, `cast_ops.md`, `float_ops.md`,
`i128_ops.md`, and `f128.md`.

## Watchouts

- Do not mechanically convert markdown shards into same-named C++ files.
- Do not reopen the shared MIR printer boundary closed by idea 224.
- Treat `records.md` as already represented only for the narrow facts proven by
  current compiled owners; its future structured asm/object roadmap text is not
  a Step 3 implementation mandate.
- Do not recreate the stale `asm_emitter.md` inline-asm emitter, scratch
  allocator, template substitution path, or text-first assembler handoff.
- Do not classify a missing feature before checking current compiled owners.
- Calls and frames have prepared authority but need AArch64 machine-node and
  printer work; do not re-run local ABI classification in target codegen.
- Full variadic function-entry is a prepared/shared carrier gap before it is an
  AArch64 codegen gap; keep it separate from the existing variadic call-boundary
  fields.
- Global memory access support is not the same as global, label, GOT, or TLS
  address materialization.
- Keep compatibility projection out of terminal assembly printing; terminal
  assembly must walk shared `module::MachineModule` through
  `mir::print_machine_module` plus the AArch64 `MachineInstructionPrinter`.

## Proof

Ledger-only classification; no build or test proof required, and no broad
validation was run. No `test_after.log` was produced because the delegated
proof explicitly required ledger-only classification.

Evidence inspected with focused `rg`, `sed`, and `nl` reads of:

- `ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md`
- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/codegen/variadic.md`
- `src/backend/mir/aarch64/codegen/globals.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` / `.hpp`
- `src/backend/mir/aarch64/module/module.cpp` / `.hpp`
- `ideas/open/231_aarch64_call_frame_machine_nodes.md`
- `ideas/open/232_aarch64_variadic_function_entry_carriers.md`
- `ideas/open/233_aarch64_global_address_materialization.md`
