Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Label, Branch, Jump, And Relocation Support

# Current Packet

## Just Finished

Step 3 - Add Label, Branch, Jump, And Relocation Support: completed the bounded
local B-type branch-label slice for `c4c-as`. The shared RV64 line assembler now
parses and encodes `beq`, `bne`, `blt`, `bge`, `bltu`, and `bgeu` as B-type
instructions, while `c4c-as` records text offsets for labels/instructions,
resolves same-file text labels to PC-relative branch byte offsets, rejects
undefined branch targets, and rejects out-of-range or misaligned branch offsets.
The object path now preserves non-global local text labels as local object
symbols while keeping the `.globl` function label anchored at the first text
instruction.

## Suggested Next

Continue Step 3 with a bounded `jal` local-label slice in `c4c-as`: parse and
encode same-file `jal rd, label` targets with range/alignment checks, keep
external relocation forms fail-closed, and add focused byte-level tests before
touching broader relocation/object semantics.

## Watchouts

- Current unsupported assembler gaps are `jal` label targets and external
  relocation/fixup semantics; the broad corpus now advances past B-type branch
  labels and still fails closed at the first unsupported `jal` label use without
  writing an object.
- c4c-objdump does not yet decode full RV64I, so the roundtrip contract must
  remain fail-closed until Step 3 and Step 4 support land.
- `backend_rv64_roundtrip_contract` currently passes by requiring the broad
  corpus to fail closed at pass1 without writing an object. When assembler
  support expands, the same script is already structured to continue through
  objdump/as/objdump/as and assert text/object stability.
- Keep unsupported extensions, unsupported directives, `jal` labels, and
  external branch/jump targets fail-closed; do not use external assembler or
  objdump output as the source of truth.

## Proof

Passed:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(c4c_as|c4c_objdump|cli_riscv64|rv64)'
```

Proof log: `test_after.log`.
