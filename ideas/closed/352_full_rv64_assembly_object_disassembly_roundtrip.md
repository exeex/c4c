# Full RV64 Assembly/Object/Disassembly Roundtrip

## Goal

Expand the c4c assembler and objdump path from the minimal EV64 smoke subset
into a complete RV64 assembly roundtrip route.

End state:

```text
any supported RV64 .s
  -> c4c-as -> .o
  -> c4c-objdump -> canonical .s
  -> c4c-as -> .o
  -> c4c-objdump -> canonical .s
  -> c4c-as -> .o
```

After two canonicalization passes, the final two object files must be byte-for-
byte identical for the covered RV64 instruction corpus.

## Dependency

This is a follow-up to the minimal bring-up ideas:

- `ideas/closed/349_rv64_single_line_assembler_core_for_inline_asm.md`
- `ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md`
- `ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md`

Ideas 350 and 351 may validate only the subset that exists at the time. This
idea is the later completeness push and must not be treated as already covered
by the minimal `.insn.d`, `li`, and `ret` proof.

## Scope

Support the RV64 assembly surface needed for a broad RV64 instruction corpus:

- RV64I base integer instructions
- common pseudoinstructions that should canonicalize to supported real
  encodings or a stable canonical pseudo spelling
- register aliases and canonical register printing policy
- immediates, signedness, and range checks
- labels, local labels, branches, jumps, and relocatable fixups needed by the
  chosen corpus
- `.text` and the minimum directives required by the corpus
- c4c EV64 `.insn.d` custom instruction decode/encode alongside ordinary RV64
  instructions

The exact supported extension set should be declared before implementation
starts. At minimum this must cover full RV64I. If the chosen corpus includes
M/A/F/D/C or other extension instructions, either support those extensions or
choose/derive a corpus that is explicitly RV64I plus the c4c EV64 custom
instruction.

## Required Corpus

This idea must select or build a comprehensive RV64 `.s` corpus before claiming
completion.

The corpus should include:

- at least one representative instruction from every RV64I format and major
  family
- arithmetic/logical register-register and register-immediate operations
- loads and stores across supported widths
- branches with forward and backward labels
- jumps and calls where in scope
- upper-immediate and PC-relative forms
- shift immediates and shift registers
- signed and unsigned comparisons
- edge immediates near legal boundaries
- canonical register aliases
- the c4c EV64 `.insn.d` custom instruction

The selected corpus should live in the repo as a normal test input, not only in
`/tmp`.

## Desired Roundtrip Proof

For the selected corpus:

```text
input.s
  -> c4c-as -> pass1.o
  -> c4c-objdump -> pass1.s
  -> c4c-as -> pass2.o
  -> c4c-objdump -> pass2.s
  -> c4c-as -> pass3.o
```

Acceptance requires:

- `pass1.s == pass2.s`
- `pass2.o == pass3.o`
- no external assembler or external objdump is the source of truth
- the test is part of ordinary full `ctest --test-dir build ...` output

External tools may be used only as optional diagnostics, not as the primary
encoder/decoder.

## In Scope

- Extending the RV64 single-line assembler core beyond the minimal subset.
- Extending `c4c-as` file parsing only as needed by the corpus.
- Extending `c4c-objdump` decoding and canonical printing.
- Relocation/fixup support needed for the selected RV64 corpus.
- Stable canonical output policy for pseudoinstructions versus real
  instructions.
- Full-ctest integrated roundtrip test.

## Out Of Scope

- Recovering original C/C++ source.
- Exact GNU assembler formatting preservation.
- Debug info, DWARF, source line reconstruction, or comments preservation.
- Treating unsupported extensions as silently accepted.
- Relying on `llvm-objdump` to understand c4c EV64 custom instructions.

## Acceptance Criteria

- A complete RV64I instruction corpus is committed under tests and assembled by
  `c4c-as`.
- `c4c-objdump` disassembles the resulting object into canonical `.s` without
  printing c4c EV64 custom instructions as `<unknown>`.
- The two-pass canonicalization proof passes:
  - `pass1.s == pass2.s`
  - `pass2.o == pass3.o`
- The roundtrip test is registered in the regular CTest suite and runs during
  full `ctest --test-dir build -j --output-on-failure` without special manual
  invocation.
- Unsupported instructions or directives fail closed with diagnostics instead
  of being skipped or misdecoded.

## Reviewer Reject Signals

- The implementation only proves the minimal `.insn.d`, `li`, `ret` subset and
  claims full RV64 assembler/disassembler completion.
- The corpus does not cover all declared RV64I instruction families.
- The roundtrip compares only text and never checks object stability.
- The second assembled object differs from the third assembled object.
- The test is hidden behind a manual script and is not part of normal CTest.
- External `as` or `objdump` becomes the source of truth for c4c custom
  instruction handling.
- Unknown bytes are silently skipped or printed as valid instructions without
  decoding their fields.

## Closure Notes

Closed on 2026-06-25.

Completion evidence:

- The committed RV64I plus c4c EV64 corpus and contract live under
  `tests/backend/rv64/`.
- `backend_rv64_roundtrip_contract` is registered in ordinary CTest and proves
  `input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o`.
- The roundtrip contract checks both `pass1.s == pass2.s` and
  `pass2.o == pass3.o` without using an external assembler or objdump as the
  source of truth.
- Closure review found the implementation aligned with this idea and found no
  testcase-overfit or expectation downgrade.
- Full-suite regression logs `test_before.log` and `test_after.log` both
  reported 3351/3351 passing tests. The close-time regression guard passed in
  non-decreasing mode with no new tests over 30 seconds.

Known boundaries retained intentionally:

- Unsupported external control-flow relocation semantics remain fail-closed.
- Unsupported extensions and directives continue to fail closed with
  diagnostics rather than being silently accepted.
