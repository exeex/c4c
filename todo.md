Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Remaining Failure Families

# Current Packet

## Just Finished

Step 2: Classify Remaining Failure Families completed from the
supervisor-captured broad backend baseline in `test_before.log` plus existing
generated assembly under `build/c_testsuite_aarch64_backend/`.

Strongest focused semantic owner candidate:

- AArch64 global/static symbol-address materialization must preserve address
  values in 64-bit registers through `adrp`/`:lo12:` address formation before
  `ldr`/`str`.
- In-scope ready cases: `00050.c`, `00176.c`, and `00182.c`.
- Shared evidence: the assembler rejects generated code where the address
  temporary is a 32-bit `wN` register, then the same `wN` is used as a memory
  base. Examples from existing assembly are `00050.c.s:44-46` (`adrp w9,
  v+8`; `ldr w9, [w9]`), `00176.c.s:271-276` (`adrp w10, array+56`;
  `ldr w10, [w10]`; `adrp w9, array+60`; `ldr w9, [w9]`), and
  `00182.c.s:620-625` (`adrp w10, __static_local_print_led_3+120`;
  `ldr w10, [w10]`; `adrp w9, __static_local_print_led_3+124`;
  `ldr w9, [w9]`).
- Distinct from closed owners: this is not fused compare-branch operand
  publication, scalar immediate fallback, scalar-cast spelling, memory-store
  source materialization, or scalar selected-node operand forms. The bad
  operation is address-register width selection for symbol+offset memory
  references after code generation has already emitted assembly.
- Tractability signal: all three in-scope cases fail before runtime with the
  same assembler legality symptom and have concrete generated assembly
  evidence. A focused owner can be proved by rebuilding and rerunning these
  named backend c-tests plus nearby symbol+offset/global/static array cases,
  without expectation, allowlist, timeout, runner, CTest-registration, or
  unsupported-classification changes.

## Suggested Next

Suggested Next: split a focused owner for AArch64 symbol+offset address
materialization/register-width legality covering `00050.c`, `00176.c`, and
`00182.c`. Do not include `00189.c` unless additional evidence shows the same
address-materialization repair also covers externally binding GOT/PIC symbol
references such as `stdout`.

## Watchouts

- Park `00189.c` as a separate externally binding symbol/PIC relocation bucket:
  generated assembly uses `adrp x10, stdout` / `add x10, x10, :lo12:stdout`,
  but link fails on `R_AARCH64_ADR_PREL_PG_HI21` against
  `stdout@@GLIBC_2.17`. That may need GOT/PIC handling, not the `w`-base
  symbol+offset fix.
- Park `00140.c` call-boundary move, `00164.c` scalar `mul` printable rhs
  facts, `00214.c` unprepared frame-slot source, and `00204.c`/`00216.c`
  semantic `lir_to_bir` admission as separate compile-stage buckets. Each has
  a distinct diagnostic and should not be bundled with symbol-address
  legality.
- Park runtime nonzero, runtime mismatch, crash, and timeout cases until there
  is focused generated-code or trace evidence. Current `test_before.log`
  mostly records empty stdout/stderr for nonzero/crash cases, output diffs for
  mismatch cases, and CTest timeouts for `00143.c`, `00187.c`, and `00220.c`;
  that is not enough to assign a semantic owner without overfitting filenames.
- Overfit/reject signals for the proposed owner: any split that matches only
  testcase names, rewrites expectations, marks cases unsupported, alters
  runner/timeout/CTest registration behavior, or treats `00189.c` as covered
  without GOT/PIC evidence should be rejected.

## Proof

No tests were rerun per packet instruction. Classification proof used the
existing `test_before.log` from `(cd build && timeout 900 ctest -j10 -R backend
--output-on-failure)` plus existing generated assembly files
`build/c_testsuite_aarch64_backend/src/00050.c.s`,
`build/c_testsuite_aarch64_backend/src/00176.c.s`,
`build/c_testsuite_aarch64_backend/src/00182.c.s`, and
`build/c_testsuite_aarch64_backend/src/00189.c.s`. `test_after.log` was not
modified.
