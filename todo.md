Status: Active
Source Idea Path: ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Unsigned Div Rem Producer Boundary

# Current Packet

## Just Finished

Step 1 localized the unsigned div/rem producer-publication boundary for
`ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md`.

First bad boundary: producer instruction emission and result publication for
non-power-of-two unsigned div/rem. The AArch64 scalar ALU path has focused
power-of-two reductions for `UDiv`/`URem`, but ordinary non-power unsigned
division/remainder do not become printable scalar ALU publication nodes:
`is_scalar_alu_publication_opcode(...)` includes integer ALU, `Mul`, `SDiv`,
and `SRem`, but not `UDiv`/`URem`; the existing prepared scalar ALU records
test explicitly expects non-power unsigned reductions to fail closed.

Representative generated evidence:

- Unsigned remainder consumer: `tests/c/external/c-testsuite/src/00182.c`
  contains `d[n++] = (int)(x%10L);`. In
  `build/c_testsuite_aarch64_backend/src/00182.c.s`, the corresponding loop
  has no `udiv`/`msub` remainder synthesis. Around lines 547-570 it loads
  `x`, sets `w20` from `cmp x13, #0`, then does `mov w13, w20` before storing
  `w13` through selected global `d[]` stores. That means the selected-store
  value handoff is already consuming stale condition state because the `URem`
  producer never emitted/published a remainder result.
- Unsigned division consumer: the same source loop contains `x = x/10L;`.
  Around lines 943-970 of the generated assembly, the loop update path checks
  `n >= 32`, then reaches `ldr x13, [sp]` followed by `str x20, [sp]` and
  branches back to the loop header. There is no `udiv` instruction and no
  publication of a quotient value into the stack slot for `x`, so the loop
  update consumer receives stale `w20`/`x20` state instead of `x / 10`.

Boundary classification: not truncation handoff, selected-store value handoff,
or loop-update consumer handoff as first bad fact. Those consumers are visibly
bad in `00182`, but both are downstream of the missing non-power `UDiv`/`URem`
producer emission/publication. Remainder synthesis after `udiv` is also
missing because the `udiv` producer itself is not emitted for the non-power
case.

## Suggested Next

Execute Step 2 by adding focused backend coverage for ordinary non-power
unsigned div/rem producers feeding scalar consumers before using `00182` as
external proof. Suggested narrow subset for Step 2/3:
`ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_scalar_alu_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00182_c)$'`.

## Watchouts

- Do not special-case `00182`, the LED digit array, a temporary name, one
  register, or one emitted instruction sequence.
- Do not widen into recursive call argument preservation for `00176`/`00181`
  or indexed aggregate selected-address/writeback from idea 348.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Keep the Step 2 focused tests semantic: ordinary `UDiv` should produce a
  quotient consumed by a scalar use, and ordinary `URem` should synthesize and
  publish a remainder consumed by a scalar use. Selected-store/`00182` should
  remain representative external proof after that core producer boundary is
  covered.

## Proof

No build or test rerun was required for this localization packet. Evidence came
from existing generated assembly
`build/c_testsuite_aarch64_backend/src/00182.c.s`, source
`tests/c/external/c-testsuite/src/00182.c`, and focused read/AST-backed symbol
inspection of the AArch64 scalar ALU publication path. Ran `ctest --test-dir
build -N` only to identify the proposed narrow Step 2/3 subset; no root-level
logs were modified.
