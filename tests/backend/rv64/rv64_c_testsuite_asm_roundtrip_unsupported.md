# RV64 c-testsuite asm/objdump roundtrip unsupported cases

This file records why nearby c-testsuite cases are not in
`rv64_c_testsuite_asm_roundtrip_allowlist.txt` yet. The allowlist is an
opt-in manifest, not a claim that every unlisted c-testsuite case has already
been classified.

## Included baseline

- `src/00001.c`: minimal `return 0`; currently exercises the direct backend,
  assembler, and objdump roundtrip without local storage or pseudo-instruction
  lowering gaps.
- `src/00002.c`: minimal constant expression return; keeps the first scan
  broad enough to prove more than one input while staying inside the same
  known-supported lowering family as `src/00001.c`.

## Known unsupported families

### Local-variable return path requiring `mv`

- Representative case: `src/00003.c`.
- Current failure stage: `c4c-as-pass1`.
- Current failing assembly shape: generated RV64 assembly contains
  unsupported pseudo-instruction `mv a0, s1`.
- Status: excluded until the assembler accepts `mv` as a pseudo-instruction or
  the backend prints an equivalent supported base instruction such as
  `addi a0, s1, 0`.

This family appears as soon as the source case needs a local variable value to
be returned through a register move. It should be rechecked before adding
pointer or local-storage-heavy cases such as later `src/0000*.c` files to the
allowlist.
