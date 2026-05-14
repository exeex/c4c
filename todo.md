# Current Packet

Status: Active
Source Idea Path: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tie Structured Records To Prepared And Machine-Level Facts

## Just Finished

Completed Step 4 from `plan.md`: tied structured asm/encoding records to
prepared lifecycle authority and post-selection machine-level effects in the
accepted AArch64 contract docs.

Updated contract docs:

- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`: added the
  prepared authority boundary for liveness, regalloc, move resolution,
  spill/reload, preserved-value, and clobber facts; clarified that structured
  asm/encoding records add only post-selection implicit register uses/defs,
  selected opcode clobbers, flags, scratch lifetimes, operator effects, and
  section/relocation ownership.
- `src/backend/mir/aarch64/codegen/records.md`: mirrored the same prepared
  authority and post-selection effect split for future record-core work while
  preserving selected machine nodes as the semantic boundary.
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`: recorded the
  proceed/split boundary that prepared carriers remain authority and lower
  structured records only add machine-level facts that exist after opcode
  selection.

## Suggested Next

Execute the next supervisor-selected plan step after Step 4, keeping the
structured node/record boundary intact and staying in docs or code only as
delegated.

## Watchouts

- Preserve the separation between terminal printer output, external assembler
  input, structured encoder/object records, and linker object inputs.
- Prepared lifecycle facts remain upstream authority; do not let future
  structured asm/encoding records duplicate allocation, liveness,
  call-preservation, or spill/reload policy.
- Future behavior work must attach post-selection machine effects to selected
  machine nodes or derived structured records, not to printed assembly text or
  parser-recovered operands.
- Do not edit the source idea for routine execution details; record packet
  state here instead.

## Proof

Delegated proof completed successfully and wrote `test_after.log`:

```bash
bash -lc 'set -o pipefail; rg -n "PreparedLiveness|PreparedLiveInterval|PreparedRegalloc|PreparedRegallocValue|PreparedInterferenceEdge|PreparedMoveResolution|PreparedSpillReloadOp|PreparedValueLocations|PreparedCallPreservedValue|PreparedClobberedRegister|implicit register|selected opcode clobber|scratch|section/relocation ownership" src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md src/backend/mir/aarch64/codegen/records.md src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md todo.md > test_after.log 2>&1; test -s test_after.log'
```

Proof result: required prepared-authority and post-selection machine-effect
contract vocabulary is present.
