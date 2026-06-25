# VRM Regalloc, MIR, And RV64 Assembler Bridge

## Goal

Carry source-level `__c4c_builtin_vrm1/2/4/8` values past the completed
regalloc-frontier work into target-consumable MIR/assembler object emission.

The intended end state is a source-level custom vector inline asm case where
VRM carrier values allocate legal contiguous vector-register groups, substitute
their base register into `.insn.d`, and emit the expected RV64 object bytes
through c4c's native route.

This is the follow-up to:

- `ideas/closed/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md`
- `ideas/closed/346_rv64_standard_insn_scalar_inline_asm_object_route.md`
- `ideas/closed/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`

## Hard Rule: No Vector ABI

There is no ordinary function-call ABI for VRM carrier values.

- A real call must not pass or return `__c4c_builtin_vrm*`.
- Inline/template functions may mention VRM parameters only if frontend
  expansion removes the call boundary before backend handoff.
- If a VRM value reaches call lowering as an argument, return value, indirect
  call operand, or function-pointer signature, compilation must diagnose the
  error.
- Do not invent, assume, or borrow an RV64 vector calling convention to make a
  testcase pass.

## Scope

This umbrella should split into focused child ideas when activated. The
expected sequence is:

1. **VRM Final Allocation And MIR Carrier**
   - Extend allocation proof from "frontier metadata exists" to actual
     assigned physical vector register groups.
   - Support `VRM1`, `VRM2`, `VRM4`, and `VRM8`.
   - Enforce aligned bases:
     - `VRM1`: any `vN`
     - `VRM2`: `v0,v2,...,v30`
     - `VRM4`: `v0,v4,...,v28`
     - `VRM8`: `v0,v8,v16,v24`
   - Treat each group as occupying every physical unit in the span.
   - Preserve base register identity for inline asm substitution and
     instruction encoding.
   - Keep tied operand semantics explicit: tied/register-matching operands may
     reuse the same group; independent operands must not overlap.

2. **Target-Neutral VRM Spill/Reload Pseudo Contract**
   - Define prepared/MIR pseudo spill and reload records for grouped vector
     registers.
   - The pseudo must carry:
     - value id / source value name when available
     - register bank and class
     - contiguous width
     - base register name
     - occupied register names
     - structured spill slot id and offset
   - The pseudo must not be rendered as target assembly text at the prepared
     layer.
   - Target MIR owns lowering pseudo spill/reload to real target load/store or
     custom vector memory instructions.

3. **RV64 Inline Asm MIR Operand Substitution**
   - Consume prepared inline asm carriers and VRM allocation results in the RV64
     MIR/object path.
   - Substitute grouped VRM operands as the base vector register (`vN`).
   - Ensure `.insn.d` encodes the base physical register index, not every unit
     in the group.
   - Keep scalar `.insn r` behavior from the closed scalar child intact.

4. **RV64 Assembler/Object Bridge**
   - Route substituted `.insn` / `.insn.d` forms through c4c-owned assembly or
     object emission.
   - Prove source-level `__c4c_builtin_vrm*` inline asm emits expected object
     bytes without relying on an external assembler as the primary proof.
   - If textual `.s` emission is involved, keep it as a parallel inspectable
     path, not as the only source of truth.

## In Scope

- Source-level local VRM carrier values used by inline asm.
- Final allocation of contiguous vector register groups.
- `VR`, `VRM1`, `VRM2`, `VRM4`, and `VRM8` constraint handling where these
  names map to the same register-footprint model.
- Target-neutral pseudo spill/reload records for grouped vector registers.
- RV64 MIR/object consumption of prepared VRM allocation and inline asm
  carriers.
- Object-byte proof for at least one `.insn.d` route using source-level
  `__c4c_builtin_vrm2` or wider carriers.

## Out Of Scope

- Function-call ABI for VRM carriers.
- RVV/SVE semantic intrinsic lowering.
- Compiler-known EV mnemonics or operation-specific builtins.
- Named GNU operands, `%c[...]` modifiers, masks, or broader GNU assembler
  compatibility unless a later source idea opens them.
- Disassembler support.
- Relaxation or relocation support for custom vector instruction payloads.
- General memory layout semantics for VRM carriers beyond structured spill
  slots and explicit future load/store pseudo/intrinsic work.

## Acceptance Criteria

- A source-level local VRM inline asm case reaches prepared/MIR with assigned
  vector register groups, not only prealloc metadata.
- `VRM2`, `VRM4`, and `VRM8` allocation tests prove aligned base selection and
  non-overlap against adjacent live groups.
- Tied VRM operands prove intentional reuse of the same group.
- Spill/reload pseudo records carry structured group and spill-slot authority
  without target assembly text embedded in the prepared layer.
- RV64 inline asm substitution uses the base vector register for grouped
  operands.
- A source-level `.insn.d` case using `__c4c_builtin_vrm*` emits deterministic
  RV64 object bytes through c4c's route.
- Scalar `.insn r` object-route tests from the closed scalar child remain
  green.
- Non-expanded VRM calls remain diagnostics and are not weakened to
  unsupported expectations or fallback ABI behavior.

## Suggested Proof Ladder

1. Frontend/BIR proof:
   - `__c4c_builtin_vrm*` locals lower to BIR VRM types.
   - Non-expanded call-boundary diagnostics still fire.

2. Prepared/regalloc proof:
   - liveness values classify as vector width 1/2/4/8.
   - allocated homes have legal aligned base and occupied names.

3. MIR contract proof:
   - MIR records expose base register, occupied group, and pseudo spill/reload
     data as structured records.

4. RV64 object proof:
   - `.insn.d` source inline asm with VRM operands emits the expected 8-byte
     EV/custom instruction payload.
   - The proof checks bytes directly with repo-native object inspection.

5. Regression proof:
   - Run a targeted backend/MIR/RV64 subset first.
   - Escalate to the matching broader regression guard before closing the
     umbrella or treating the bridge as milestone-complete.

## Reviewer Reject Signals

- The implementation makes scalar `long` or `i64` values satisfy `VRM*`
  constraints.
- `VRM2`, `VRM4`, or `VRM8` can allocate misaligned bases such as `v1-v2` for
  `VRM2` or `v2-v5` for `VRM4`.
- Independent VRM operands overlap without a tied/matching constraint.
- Spill/reload is emitted as ad hoc target asm strings before MIR owns the
  target lowering.
- The object proof uses fixture-built prepared modules only, while real
  source-level VRM carrier lowering is absent.
- The route relies on an external assembler as the primary proof of custom
  instruction bytes.
- Any vector function-call ABI is invented or silently used.
- Tests are weakened, marked unsupported, or rewritten around the exact sample
  instead of proving the general carrier/allocation/substitution route.
