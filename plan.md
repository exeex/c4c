# VRM Regalloc, MIR, And RV64 Assembler Bridge Runbook

Status: Active
Source Idea: ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md

## Purpose

Carry source-level `__c4c_builtin_vrm1/2/4/8` values past the completed
regalloc-frontier work into target-consumable MIR and RV64 assembler/object
emission.

Goal: make a source-level custom vector inline asm case allocate legal
contiguous vector-register groups, substitute the base register into `.insn.d`,
and emit deterministic RV64 object bytes through c4c's native route.

## Core Rule

Do not invent or use a function-call ABI for VRM carrier values. A VRM value
that reaches call lowering as an argument, return value, indirect call operand,
or function-pointer signature must diagnose instead of falling back to scalar,
unsupported, or target ABI behavior.

## Read First

- `ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md`
- Closed source ideas referenced by that file when historical route context is
  needed:
  - `ideas/closed/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md`
  - `ideas/closed/346_rv64_standard_insn_scalar_inline_asm_object_route.md`
  - `ideas/closed/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`

## Current Targets And Scope

- Source-level local VRM carrier values used by inline asm.
- Final allocation of contiguous vector register groups for `VR`, `VRM1`,
  `VRM2`, `VRM4`, and `VRM8` where these names share the same
  register-footprint model.
- Target-neutral prepared/MIR pseudo spill and reload records for grouped
  vector registers.
- RV64 MIR/object consumption of prepared VRM allocation and inline asm
  carriers.
- Object-byte proof for at least one `.insn.d` route using source-level
  `__c4c_builtin_vrm2` or wider carriers.

## Non-Goals

- Do not add a function-call ABI for VRM carriers.
- Do not implement RVV/SVE semantic intrinsic lowering.
- Do not add compiler-known EV mnemonics or operation-specific builtins.
- Do not add named GNU operands, `%c[...]` modifiers, masks, or broader GNU
  assembler compatibility unless a later source idea opens that scope.
- Do not add disassembler, relaxation, or relocation support for custom vector
  instruction payloads.
- Do not define general memory layout semantics for VRM carriers beyond
  structured spill slots and explicit future load/store pseudo/intrinsic work.

## Working Model

- `VRM1` may use any vector base register.
- `VRM2` must use an even base register: `v0`, `v2`, ..., `v30`.
- `VRM4` must use a multiple-of-four base register: `v0`, `v4`, ..., `v28`.
- `VRM8` must use `v0`, `v8`, `v16`, or `v24`.
- Each group occupies every physical vector register in its span.
- Inline asm substitution uses the group base register (`vN`), not every
  occupied register.
- Tied or register-matching operands may intentionally reuse the same group.
  Independent operands must not overlap.
- Prepared-layer spill/reload data must be structured records, not embedded
  target assembly text.

## Execution Rules

- Prefer semantic allocation, MIR, and object-emission capability over
  testcase-shaped matching.
- Keep scalar `.insn r` object-route behavior from the closed scalar child
  green while adding vector support.
- For each code-changing step, run a fresh build or compile proof plus the
  delegated narrow test subset.
- Escalate to a broader backend/MIR/RV64 regression guard before treating this
  bridge as milestone-complete.
- Do not weaken supported-path tests to unsupported expectations without
  explicit user approval.
- If a step becomes too broad, ask the supervisor to split the runbook or
  source idea instead of expanding scope ad hoc.

## Ordered Steps

### Step 1: Frontier And Call-Boundary Baseline

Goal: establish the current VRM carrier frontier and preserve the no-vector-ABI
diagnostic contract before final allocation work starts.

Primary targets:
- frontend/BIR VRM carrier lowering
- call-lowering diagnostics for non-expanded VRM arguments and returns
- existing regalloc-frontier metadata tests

Actions:
- Inspect source-level `__c4c_builtin_vrm1/2/4/8` local lowering into BIR and
  prepared/regalloc metadata.
- Identify the narrow tests that prove local VRM carriers reach the regalloc
  frontier.
- Identify or add diagnostic coverage that proves real calls, indirect calls,
  returns, and function-pointer signatures with VRM carriers are rejected.
- Record the exact narrow proof command in `todo.md` when the executor runs it.

Completion check:
- Local VRM carrier lowering is still observable before final allocation work.
- Non-expanded VRM call boundaries diagnose instead of using fallback ABI
  behavior.
- No expectation is downgraded to unsupported to make the baseline pass.

### Step 2: Final VRM Allocation And MIR Carrier

Goal: allocate physical contiguous vector-register groups for VRM values and
preserve base-register identity for later MIR and inline asm consumption.

Primary targets:
- regalloc homes for vector register classes
- liveness and interference for grouped physical registers
- tied operand handling for inline asm carriers

Actions:
- Extend allocation from frontier metadata to assigned physical vector homes.
- Support `VR`, `VRM1`, `VRM2`, `VRM4`, and `VRM8` through one footprint model.
- Enforce legal aligned base selection for widths 1, 2, 4, and 8.
- Mark every physical vector register in a group as occupied for interference.
- Preserve the base register name/index separately from the occupied-register
  list.
- Keep tied/register-matching operands explicit so reuse is allowed only when
  the constraint requires it.

Completion check:
- Tests prove aligned base selection and non-overlap for `VRM2`, `VRM4`, and
  `VRM8`.
- A tied VRM operand test proves intentional group reuse.
- Independent VRM operands cannot overlap.
- Allocation results expose both base register and occupied group identity for
  downstream MIR use.

### Step 3: Target-Neutral Spill/Reload Pseudo Contract

Goal: define prepared/MIR pseudo records for spilling and reloading grouped
vector registers without rendering target assembly at the prepared layer.

Primary targets:
- prepared/MIR spill and reload representation
- structured spill slot id and offset data
- grouped vector register metadata

Actions:
- Define pseudo spill/reload records that carry value id or source value name
  when available.
- Carry register bank, register class, contiguous width, base register name,
  occupied register names, structured spill slot id, and offset.
- Keep pseudo records target-neutral until target MIR lowering owns the real
  load/store or custom vector memory instruction.
- Add tests or dumps that verify the structured pseudo contract without
  depending on target asm strings.

Completion check:
- MIR records expose group and spill-slot authority as structured data.
- Prepared-layer output does not contain ad hoc target asm strings for grouped
  spill/reload.
- The contract remains usable by later RV64 target lowering.

### Step 4: RV64 Inline Asm MIR Operand Substitution

Goal: consume prepared inline asm carriers and VRM allocation results in the
RV64 MIR/object path.

Primary targets:
- RV64 MIR inline asm operand lowering
- `.insn.d` operand substitution
- scalar `.insn r` route preservation

Actions:
- Substitute grouped VRM operands as their base vector register string/index.
- Ensure `.insn.d` encoding consumes the base physical register index, not the
  full occupied group.
- Preserve scalar `.insn r` behavior and tests from the closed scalar object
  route.
- Reject scalar `long` or `i64` values as satisfying `VRM*` constraints.

Completion check:
- RV64 inline asm substitution uses `vN` base registers for grouped VRM
  operands.
- `.insn.d` encodes the base physical vector register index.
- Scalar `.insn r` object-route tests remain green.
- Scalar integer values do not satisfy `VRM*` constraints.

### Step 5: RV64 Assembler/Object Bridge And Milestone Proof

Goal: prove source-level VRM inline asm reaches deterministic RV64 object bytes
through c4c-owned emission.

Primary targets:
- RV64 c4c-owned assembly/object emission
- object-byte inspection for custom vector `.insn.d`
- backend/MIR/RV64 regression guard

Actions:
- Route substituted `.insn` and `.insn.d` forms through c4c-owned assembly or
  object emission.
- Keep textual `.s` emission, if used, as an inspectable parallel path rather
  than the only source of truth.
- Add or identify a source-level `.insn.d` test using
  `__c4c_builtin_vrm2` or wider carriers.
- Check emitted object bytes directly with repo-native object inspection.
- Run the targeted backend/MIR/RV64 subset first, then escalate to a matching
  broader regression guard before closure or milestone-complete treatment.

Completion check:
- A source-level VRM `.insn.d` case emits the expected custom instruction
  bytes through c4c's native route.
- The proof does not rely on an external assembler as the primary authority.
- Scalar `.insn r` object-route tests remain green.
- The broader regression guard is green or any failures are documented as
  pre-existing by matching baseline logs.
