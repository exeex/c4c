Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the Object-Emission Handoff Contract

# Current Packet

## Just Finished

Step 2 contract audit completed without backend implementation edits.

Current object-route contract:

- Input entrypoint:
  `build_rv64_prepared_text_object_module(const PreparedBirModule&)` at
  `src/backend/mir/riscv/codegen/object_emission.cpp:1633`-`1651`.
- Current conversion:
  each prepared control-flow function is converted by
  `prepared_function_to_object_function` into `RiscvObjectFunction`.
- Current output data shape:
  `RiscvObjectFunction { name, global, vector<RiscvEncodedFragment> fragments }`
  in `src/backend/mir/riscv/codegen/object_emission.hpp:38`-`42`.
- Current fragment shape:
  `RiscvEncodedFragment { bytes, labels, fixups }` in
  `object_emission.hpp:32`-`36`. This is already a low-level encoded bytes
  contract, not a target-block contract.
- Current object writer:
  `build_rv64_text_object_module` appends fragment bytes, binds labels, and
  attaches relocations at `object_emission.cpp:1548`-`1627`.
- Current fatal mismatch:
  `prepared_function_to_object_function` rejects
  `function->blocks.size() != 1` at `object_emission.cpp:936`-`940`, then reads
  only `function->blocks.front()` at `object_emission.cpp:946`. So the object
  route cannot represent per-block labels, per-block terminators, or
  predecessor-edge copy obligations even though prepared BIR already has them.

Step 2 handoff contract:

```text
PreparedBirModule
  -> defined PreparedControlFlowFunction only
  -> Rv64PreparedObjectFunction
       name/global
       ordered blocks matching BIR/prepared block order
       per-block stable local label spelling
       entry-only prologue/formal-home fragments
       per-block instruction fragments selected from prepared BIR + prepared lookups
       explicit predecessor-edge publication/copy fragments, attached to the
         predecessor terminator site described by prepared control-flow metadata
       explicit terminator fragments:
         Return -> epilogue + ret
         Branch -> local-label jump/fixup
         CondBranch -> fused prepared compare-branch or fail-closed diagnostic
       function-level symbol extent from first emitted byte to final emitted block byte
  -> ObjectModule .text bytes + labels + relocations
  -> ELF object
```

Contract rules:

- Object emission consumes prepared BIR plus prepared lookups as the authority
  for block order, labels, branch conditions, call plans, register homes, and
  edge-copy obligations. It must not infer if/loop/switch/liveness from bytes or
  textual assembly.
- The handoff may keep using `RiscvEncodedFragment` for already-selected
  machine bytes/fixups, but it needs a block-aware container above fragments.
  Removing the single-block gate without adding block identity would be the
  wrong fix.
- Branch and jump terminators must use explicit target label references. The
  encoding layer may lower local label references to immediate/fixup/relocation,
  but it must not reconstruct CFG semantics.
- Declarations must be skipped like the `.s` emitter does at
  `src/backend/mir/riscv/codegen/prepared_module_emit.cpp:160`-`171`, not
  treated as malformed defined functions.
- Globals/string/data-section support remains out of scope for 356 and should
  continue to fail closed for idea 357.
- The first implementation should target the clean representative
  `src/20000205-1.c`: it is multi-block, has no globals/strings, and Step 1
  showed the first object-route blocker is the single-block gate.

## Suggested Next

Execute Step 3 as a narrow implementation packet:

- Introduce a block-aware RV64 object emission shape, or locally equivalent
  builder, above `RiscvEncodedFragment`.
- Skip declaration-only prepared control-flow entries before conversion.
- Replace the blanket `function->blocks.size() != 1` gate with fail-closed
  per-block conversion.
- Start with unconditional branch / conditional branch / return terminator
  fragments and explicit local labels, reusing the prepared `.s` emitter logic
  as the behavioral guide.
- Keep unsupported external calls, globals/strings, missing branch condition
  forms, and unsupported instruction fragments fail-closed with precise
  diagnostics where practical.
- Prove with the narrow allowlist, but treat `20000205-1.c` as the first real
  356 capability target; `20010119-1.c` and `20030216-1.c` should remain
  out-of-scope blockers unless separately owned.

## Watchouts

- Do not move CFG semantics into MIR, assembler, or object emission.
- Do not ask the assembler to understand loops, if/else, switch, liveness, or
  inline-asm operand relationships.
- If a representative case is blocked by globals/strings/data sections, mark it
  as an idea 357 blocker instead of forcing it into 356.
- Do not make textual assembly the semantic IR. The `.s` emitter is useful as a
  reference for prepared-BIR traversal, label naming, and branch text, but the
  object contract should remain structured object fragments/labels/fixups.
- Do not merely append all block fragments linearly and hope labels fix it.
  Edge-publication/copy obligations must be emitted at the prepared execution
  site, usually predecessor terminators.
- The `.s` path is not a clean oracle for the four Step 1 cases yet: the first
  three hit unsupported runtime external symbols (`exit`/`abort`) and
  `20030216-1.c` hits global storage layout.
- `20010119-1.c` is no longer a multi-block representative after current
  lowering. It is useful as a declaration/external-call object-route blocker,
  but should not drive the first multi-block repair.

## Proof

Read-only audit proof. No backend implementation files changed, so no build was
required for Step 2.

Inspected:

- `plan.md`
- `todo.md`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.hpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
