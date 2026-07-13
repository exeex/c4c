Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Implement the verified LIR-to-BIR carrier boundary

# Current Packet

## Just Finished

- Step 1 now has a verified, target-independent `InlineAsmNode` carrier wired
  from the current LLVM-style `LirInlineAsmOp` through Raw and Canonical BIR.
- The carrier preserves opaque asm and aggregate constraint bytes plus the
  current side-effect/clobber fields; unsupported textual result/argument
  shapes and early `insn_r` metadata are rejected before publication.
- Builders and read-only views expose generic ordered SSA input `ValueId`s and
  distinct result `ValueId`s with coherent `InstResultDef` coordinates; a
  rejected input leaves no partial instruction behind.

## Suggested Next

- Review and commit this narrow InlineAsm carrier slice; a later LIR producer
  packet can populate its generic SSA edges once LIR has structured values.

## Watchouts

- Do not absorb the dirty broad docs/README/review slice into this packet.
- The current `LirInlineAsmOp` exposes operands only as opaque `args_str` and a
  textual result, so this packet deliberately accepts only void/no-argument
  imports rather than parsing text or inventing `ValueId`s. The BIR node itself
  already supports generic input/use and output/def edges.
- Ordinary instructions, allocation, target budgets, spill/reload, and MIR
  remain deferred.

## Proof

- Passed the supervisor-selected command; `test_after.log` contains the
  backend-enabled configure, target build, and passing
  `backend_lir_to_bir_interface` CTest result.
