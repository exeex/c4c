# BIR MIR Allocation Contract

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/closed/205_aarch64_arm_reference_layout_contract.md`
- `ideas/closed/206_prepared_memory_volatility_address_space_carrier.md`
- `ideas/closed/207_aarch64_target_register_and_instruction_record_core.md`
- `ideas/closed/211_aarch64_machine_instruction_node_contract.md`

## Goal

Define a portable contract between BIR / prepared facts and MIR codegen for
target calling convention, register pool taxonomy, physical-register
assignment, reserved MIR scratch usage, future virtual-register representation,
fixed/implicit operand constraints, and spill slot assignment.

This idea owns the boundary before machine instruction nodes. Idea 211 owns the
later boundary between MIR codegen and assembler/encoder.

## Why This Idea Exists

The AArch64 backend needs a stable handoff from semantic BIR values to
target-consumable locations before instruction selection grows further. The
same contract should also be able to carry the necessary x86/x86-64 constraints
later, even if first-pass x86 lowering intentionally treats the CISC target in
a RISC-like way for simplicity.

Without this contract, MIR codegen will be tempted to allocate registers,
invent spill slots, patch calling convention facts, and emit machine-like
instructions in whichever slice first needs them.

The intended model is:

```text
BIR / PreparedBirModule
  -> target ABI register-budget contract
  -> allocation result
  -> AArch64 target MIR records
  -> AArch64 machine instruction nodes
  -> .s printer / encoder
```

Core BIR should keep semantic value identity. It should not become a
target-specific register-allocation IR, and it should not store rendered
physical-register names as authority.

## Register And Spill Model

- The target ABI exposes finite register pools by register class and calling
  convention role. General-purpose registers and floating-point / SIMD
  registers must be modeled separately.
- Each register class must distinguish:
  - allocatable long-lived pool
  - argument / return pool
  - caller-saved temp pool
  - reserved MIR scratch pool
  - special / forbidden pool
- AArch64 special / forbidden register handling must explicitly cover stack,
  frame, link, zero, platform, and linker/veneer scratch registers where the
  ABI requires target-specific care.
- The reserved MIR scratch pool is not available for long-lived BIR value
  homes. It exists so MIR spill/reload lowering can always materialize a
  spilled operand without first needing a full register scavenger.
- The portable contract must leave room for x86/x86-64 fixed and implicit
  operand constraints, including:
  - fixed input/output registers such as accumulator/dividend pairs
  - implicit clobbers
  - condition/flags registers
  - variable-count shift registers
  - subregister aliasing and width effects
  - target ABI differences such as SysV vs Windows argument/return resources,
    shadow space, red zone, and stack alignment
- The allocation layer maps prepared/BIR values to one of:
  - a target physical register
  - a spill slot from a logically unbounded spill slot pool
  - a virtual register placeholder when a future allocator/lowering stage
    explicitly owns that representation
  - a constant, symbol, frame-slot, or other non-register prepared location
    when already represented by existing prepared facts
- The spill slot pool is unbounded at the contract level. Running out of
  physical registers is represented by assigning more values to spill slots,
  not by weakening BIR semantics or inventing rendered names.
- Spill slots are structured ids with size, alignment, register class, and
  provenance. They are not stack-pointer offsets until frame layout assigns
  concrete offsets.
- The allocation result may mark a value as spilled, but core BIR should not
  directly own target spill/reload instructions.
- Reload and store materialization belongs after allocation, as target MIR
  pseudo operations or a structured lowering plan consumed by MIR codegen.
  Later lowering turns those pseudo operations into machine instruction nodes.
- If a spilled operand must be loaded for an operation, MIR lowering first uses
  the reserved MIR scratch pool for that operand's register class. If that pool
  is exhausted, the first implementation should fail closed or open a follow-up
  register-scavenger idea rather than silently stealing a long-lived register.
- Future virtual registers may be represented in the contract, but the first
  AArch64 path should not require an LLVM-style post-instruction-selection
  virtual-register allocator. Machine instruction nodes from idea 211 should
  still prefer physical registers once allocation is complete.

## x86 Compatibility Rule

The contract must be usable by a conservative x86/x86-64 backend that chooses
not to use CISC optimizations at first. Such a backend may lower in a RISC-like
style:

```text
spilled operand -> reload into scratch or fixed register
operation       -> prefer register-register form
spilled result  -> store back to spill slot
```

This first-pass style may ignore performance opportunities such as memory
operands, complex addressing forms, partial-register optimization, and
instruction-specific CISC variants. It may not ignore correctness constraints:
calling-convention resources, fixed/implicit operands, flags, subregister
aliasing, stack alignment, and target ABI stack resources must remain
representable in the contract.

## In Scope

- Define a markdown contract under `src/backend/mir/aarch64/` for:
  - target ABI register budgets
  - register classes
  - GPR and FPR/SIMD pool separation
  - allocatable long-lived, argument/return, caller-saved temp, reserved MIR
    scratch, and special/forbidden pools
  - physical-register allocation results
  - future virtual-register placeholders
  - spill slot pool and spill slot identity
  - call-clobber and callee-save obligations
  - scratch-register policy for spill/reload lowering
  - fixed/implicit operand constraints needed by x86/x86-64
  - condition/flags register modeling for targets that expose it
  - subregister alias and width-effect notes for targets that need them
  - target ABI stack resources such as shadow space or red zone where relevant
  - where reload/store pseudo operations are introduced
  - how allocation provenance ties back to BIR / prepared ids
- Audit existing AArch64 ABI, module, and codegen record surfaces for places
  where this contract should be referenced or renamed.
- Update currently implemented `.cpp/.hpp` only as needed to make the
  allocation contract explicit and prevent ad hoc register/spill ownership.
- Update not-yet-implemented AArch64 markdown artifacts that currently imply
  MIR codegen should allocate registers or spill opportunistically inside each
  feature slice.
- Document any missing shared BIR/prepared carrier required to express
  allocation input or output. If a required shared carrier is missing, create a
  separate `ideas/open/` gap idea instead of patching around it inside AArch64.

## Out Of Scope

- Implementing a full register allocator.
- Implementing an x86/x86-64 backend.
- Implementing CISC-specific x86 optimizations such as memory-operand ALU
  folding, complex-addressing selection, or partial-register optimization.
- Implementing frame layout, concrete stack offsets, prologue/epilogue
  emission, or callee-save save/restore instruction emission.
- Emitting reload/store machine instructions.
- Completing calls, returns, variadics, inline asm, atomics, or vector
  lowering.
- Changing parser, sema, HIR, or core BIR semantics except through separately
  opened gap ideas.
- Using assembly text, rendered names, or parsed `.s` as allocation authority.

## Acceptance Criteria

- A committed AArch64 markdown contract defines the BIR/prepared-to-MIR
  allocation boundary separately from the machine-instruction-node boundary in
  idea 211.
- The contract states that physical-register budgets are finite and spill slot
  allocation is logically unbounded.
- The contract separates GPR and FPR/SIMD register classes and defines the
  calling-convention pools for allocatable long-lived, argument/return,
  caller-saved temp, reserved MIR scratch, and special/forbidden registers.
- The contract is portable enough to represent necessary x86/x86-64
  correctness constraints, including fixed/implicit operands, flags,
  subregister aliasing, and ABI stack resources, without requiring x86 CISC
  optimization in the first implementation.
- Reserved MIR scratch registers are excluded from long-lived allocation and
  assigned as the first-line resource for spill/reload materialization.
- Allocation results can represent physical-register locations and structured
  spill-slot locations, with an explicit future slot for virtual-register
  placeholders, without requiring core BIR to own AArch64-specific machine
  instructions.
- Reload/store materialization is assigned to target MIR pseudo operations or
  an equivalent structured lowering plan, not to core BIR and not to assembly
  text.
- Existing implemented AArch64 `.cpp/.hpp` surfaces are updated or explicitly
  documented so they do not silently own register allocation or spill policy.
- Missing BIR/prepared carriers discovered during the work are captured as
  separate open ideas.

## Reviewer Reject Signals

- Core BIR starts storing AArch64 physical-register names or stack offsets as
  semantic truth.
- Register allocation is scattered across scalar ALU, memory, branch, call, or
  return lowering slices without a shared allocation result contract.
- GPR and FPR/SIMD allocation are collapsed into one untyped pool.
- Caller-saved temp registers and reserved MIR scratch registers are treated as
  interchangeable without documenting which layer owns temporary use.
- The contract can only describe AArch64/RISC-V-style registers and cannot
  represent x86 fixed operands, flags, subregister aliasing, or ABI stack
  resources.
- x86 correctness requirements are dismissed as mere optimization details.
- MIR spill/reload lowering can run out of scratch registers and silently steal
  a long-lived value register without a structured register-scavenger contract.
- Spill slots are represented as rendered stack-address strings or final
  offsets before frame layout owns them.
- Reload/store behavior is emitted as assembly text before target MIR pseudo
  operations or machine instruction nodes exist.
- The route introduces a target-local workaround for missing prepared facts
  instead of opening a separate BIR/prepared gap idea.
- The idea overlaps 211 by redefining assembler/encoder handoff instead of
  focusing on BIR/prepared-to-MIR allocation.
