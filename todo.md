Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Label, Branch, Jump, And Relocation Support

# Current Packet

## Just Finished

Step 3 - Add Label, Branch, Jump, And Relocation Support: completed the bounded
local `jal rd, label` slice for `c4c-as`. The shared RV64 line assembler now
parses symbolic `jal` as a J-type jump line, sizes it as one 32-bit
instruction, and encodes it only after the symbolic target is resolved.
`c4c-as` now resolves same-file local jump labels to PC-relative JAL byte
offsets, rejects undefined jump targets, and rejects out-of-range or misaligned
jump offsets with focused fast line-assembler coverage while preserving the
existing local branch, non-label RV64I, and EV64 behavior.

## Suggested Next

Continue Step 3 with a bounded external relocation/fixup design slice for jump
and call-shaped forms, or hand off to Step 4 objdump decode coverage if the
supervisor wants the broad roundtrip contract to advance beyond the current
dump1 fail-closed boundary first.

## Watchouts

- Current unsupported assembler gaps are external relocation/fixup semantics;
  same-file local B-type branches and `jal rd, label` now assemble.
- c4c-objdump does not yet decode full RV64I, so
  `backend_rv64_roundtrip_contract` now accepts a fail-closed dump1 rejection
  with no partial dumped text. When objdump support expands, the same script
  continues through objdump/as/objdump/as and asserts text/object stability.
- Keep unsupported extensions, unsupported directives, and external branch/jump
  targets fail-closed; do not use external assembler or objdump output as the
  source of truth.

## Proof

Passed:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(c4c_as|c4c_objdump|cli_riscv64|rv64)'
```

Proof log: `test_after.log`.
