Status: Active
Source Idea Path: ideas/open/211_aarch64_machine_instruction_node_contract.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Align AArch64 Markdown Roadmap Artifacts

# Current Packet

## Just Finished

Step 4 of `plan.md` aligned the AArch64 markdown roadmap artifacts with
`MACHINE_INSTRUCTION_NODE_CONTRACT.md`.

The roadmap docs now route internal backend semantics through structured target
MIR records and AArch64 machine instruction nodes. `.s` output is documented
as printer output, parser surfaces are documented as external assembly input,
and encoder/object/linker surfaces are documented as downstream consumers of
machine nodes or lower structured encoding/object records.

The update preserved historical inventories while changing future-direction
language that could otherwise imply `codegen -> assembly text -> parse_asm ->
encoder/object writer` as an accepted internal route.

A follow-up tightening pass also aligned the remaining `codegen/*.md` rebuild
guidance blocks for ALU, comparison, memory, calls, casts, FP, prologue,
returns, variadic, atomics, intrinsics, i128, f128, globals, inline asm,
asm-emitter, and emit surfaces so future work routes through structured target
MIR and machine instruction nodes before `.s` printing or encoding/object
consumers.

## Suggested Next

Execute Step 5: add or run focused proof for the contract and implemented
records. Prefer the supervisor-selected narrow backend proof; add tests only if
there is an appropriate local pattern and no expectation downgrade is needed.

## Watchouts

- `RecordSurfaceKind::RecordOnly` remains available only as a compatibility
  spelling for target MIR/pre-node records; new tests should prefer
  `TargetMirRecord`, `MachineInstructionNode`, `EncoderInput`, or
  `ExternalAssemblerInput`.
- This slice did not edit implementation `.cpp/.hpp` files or
  `MACHINE_INSTRUCTION_NODE_CONTRACT.md`.
- `BINARY_UTILS_CONTRACT.md` still records the current compatibility
  text-first path as existing behavior, but now marks it as compatibility only,
  not the accepted AArch64 MIR rebuild route.
- Historical inventory sections still describe removed direct emission,
  mnemonic selection, and parser behavior as history; the tightened text is in
  future/rebuild guidance.
- Module display labels remain display/diagnostic fields, not lookup authority.

## Proof

`git diff --check` passed. Proof log: `test_after.log`.
