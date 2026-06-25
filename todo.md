Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Label, Branch, Jump, And Relocation Support

# Current Packet

## Just Finished

Step 2 - Generalize RV64 Assembly Parsing And Encoding: added semantic
parser/encoder support in the shared RV64 line assembler for the committed
non-label RV64I subset covering U-type `lui`/`auipc`, register-immediate
arithmetic/logical, shift-immediate including RV64 word shifts, register-register
arithmetic/logical including word forms, load/store displacement operands, and
label-free `jalr`. The c4c EV64 `.insn.d`, `li`, and `ret` paths remain
supported, and the assembler parse suite now proves representative object bytes
plus fail-closed behavior for branch/jump label fixups, unsupported extension
mnemonics, and immediate/shift range errors.

## Suggested Next

Execute Step 3 from `plan.md`: add label, branch, jump, and relocation/fixup
support needed by the committed RV64I corpus, starting with a narrow branch or
`jal` label-resolution packet and byte-level tests.

## Watchouts

- Current unsupported assembler gaps are label/fixup-dependent branch and `jal`
  forms, plus unresolved PC-relative/relocatable object semantics; the broad
  corpus still fails closed at the first branch label use and does not write an
  object.
- c4c-objdump does not yet decode full RV64I, so the roundtrip contract must
  remain fail-closed until Step 3 and Step 4 support land.
- `backend_rv64_roundtrip_contract` currently passes by requiring the broad
  corpus to fail closed at pass1 without writing an object. When assembler
  support expands, the same script is already structured to continue through
  objdump/as/objdump/as and assert text/object stability.
- Keep unsupported extensions and directives fail-closed; do not use external
  assembler or objdump output as the source of truth.

## Proof

Passed:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(c4c_as|c4c_objdump|cli_riscv64|rv64)'
```

Proof log: `test_after.log`.
