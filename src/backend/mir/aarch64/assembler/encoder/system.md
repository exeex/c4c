# AArch64 Encoder System Legacy Surface

This artifact preserves the useful structure from the removed
`system.cpp` translation unit. The old file was a commented C++ mirror of the
reference Rust encoder surface at
`ref/claudes-c-compiler/src/backend/arm/assembler/encoder/system.rs`.

## Role

The removed surface documented how the legacy assembler encoded AArch64 system
instructions: barriers, MRS/MSR system register transfers, exception
instructions, cache and TLB maintenance, generic `SYS` instructions, HINT/BTI,
and selected address-translation operations.

It did not provide active compiled C++ behavior in this tree. Every routine was
preserved as Rust-shaped pseudocode in comments, expecting operands from the
legacy assembler parser and returning a single instruction word.

## Dependencies And Operand Model

The old surface assumed these shared helpers and types from the assembler
encoder/parser layer:

- `Operand`, including `Operand::Barrier`, `Operand::Symbol`, and
  `Operand::Reg`
- `EncodeResult::Word`
- `get_reg(operands, index)` for destination/source GPR extraction
- `get_imm(operands, index)` for immediate fields
- `parse_reg_num(name)` for raw-register parsing in string-driven encoders

Several entry points accepted raw operand text in addition to parsed operands.
Those paths split strings directly to preserve architectural spellings such as
`tlbi vae1is, x0`, `ic ivau, x0`, `at s1e1r, x0`, and
`sys #op1, Cn, Cm, #op2, Xt`.

## Entry Points

The old surface listed these encoder entry points and helpers:

- `encode_dmb(operands)` and `encode_dsb(operands)`
- `encode_mrs(operands)` and `encode_msr(operands)`
- `sysreg_encoding(op0, op1, crn, crm, op2)`
- `parse_numbered_sysreg(name)`
- `parse_generic_sysreg(name)`
- `encode_svc(operands)`, `encode_hvc(operands)`, `encode_smc(operands)`,
  and `encode_brk(operands)`
- `encode_ic(raw_operands)`
- `encode_at(operands, raw_operands)`
- `encode_sys(raw_operands)`
- `encode_tlbi(operands, raw_operands)`
- `encode_bti(raw_operands)`
- `encode_hint(operands)`
- `encode_dc(operands, raw_operands)`

## Barrier Instructions

`encode_dmb` and `encode_dsb` accepted an optional barrier option as either a
barrier operand or a symbol operand. Missing operands defaulted to `sy`.

Recognized options and CRm values were:

```text
sy=1111, st=1110, ld=1101
ish=1011, ishst=1010, ishld=1001
nsh=0111, nshst=0110, nshld=0101
osh=0011, oshst=0010, oshld=0001
```

The base words were `0xd50330bf` for DMB and `0xd503309f` for DSB, with the
option encoded at `CRm << 8`.

## System Register Transfers

`encode_mrs` implemented `MRS Xt, system_reg` using base word `0xd5200000`.
`encode_msr` implemented `MSR system_reg, Xt` using base word `0xd5000000`.
Both routines expected the system-register field to be supplied as the encoded
`op0:op1:CRn:CRm:op2` value shifted into bits `[20:5]`.

The old MRS table included EL0/EL1/EL2 architectural registers and status
registers such as:

```text
sp_el0, sp_el1, sp_el2
tpidr_el0, tpidr_el1, tpidr_el2, tpidrro_el0
tcr_el1, ttbr0_el1, ttbr1_el1, sctlr_el1, vbar_el1
mair_el1, amair_el1, contextidr_el1, par_el1
currentel, elr_el1, spsr_el1, esr_el1, far_el1
mpidr_el1, midr_el1, revidr_el1, ctr_el0
id_aa64pfr0_el1, id_aa64pfr1_el1
id_aa64dfr0_el1, id_aa64dfr1_el1
id_aa64isar0_el1, id_aa64isar1_el1, id_aa64isar2_el1
id_aa64mmfr0_el1, id_aa64mmfr1_el1, id_aa64mmfr2_el1
hcr_el2, cptr_el2, hstr_el2, hacr_el2, vpidr_el2, vmpidr_el2
elr_el2, esr_el2, far_el2, hpfar_el2, spsr_el2
sctlr_el2, mdcr_el2, tcr_el2, ttbr0_el2, vttbr_el2, vtcr_el2
cntfrq_el0, cntpct_el0, cntvct_el0
cntv_ctl_el0, cntp_ctl_el0, cntv_cval_el0, cntp_cval_el0
pmuserenr_el0, pmcr_el0, pmcntenset_el0, pmcntenclr_el0
pmovsclr_el0, pmselr_el0, pmceid0_el0, pmceid1_el0
pmccntr_el0, pmxevtyper_el0, pmxevcntr_el0, pmccfiltr_el0
daif, fpcr, fpsr, nzcv, spsel
```

The MSR table overlapped heavily with MRS but omitted several read-only
identification and counter registers. Rebuilding this surface should keep
readable and writable register sets distinct instead of assuming table
symmetry.

## Generic System Register Parsing

`sysreg_encoding` packed fields as:

```text
((op0 & 3) << 14) | ((op1 & 7) << 11) |
((CRn & 0xf) << 7) | ((CRm & 0xf) << 3) | (op2 & 7)
```

`parse_generic_sysreg` accepted explicit names of the form:

```text
s<op0>_<op1>_c<CRn>_c<CRm>_<op2>
```

Before parsing that generic spelling, it delegated to
`parse_numbered_sysreg`, which recognized numbered register families:

- `dbgbcr<n>_el1`, `dbgbvr<n>_el1`, `dbgwcr<n>_el1`, and `dbgwvr<n>_el1`
  for `n <= 15`
- `pmevcntr<n>_el0` and `pmevtyper<n>_el0` for `n <= 30`

The performance-monitor family derived `CRm` from `8 + n / 8` for counters and
`12 + n / 8` for event type registers, with `op2 = n % 8`.

## MSR Immediate Forms

`encode_msr` first checked PSTATE-style immediate forms before falling back to
the register form:

- `msr daifset, #imm` used base `0xd5034000`, `op2 = 110`, `Rt = 11111`
- `msr daifclr, #imm` used base `0xd5034000`, `op2 = 111`, `Rt = 11111`
- `msr spsel, #imm` used base `0xd5004000`, `op2 = 101`, `Rt = 11111`

The immediate was masked to four bits. If `spsel` did not parse as an
immediate, the old code fell through to the normal `MSR system_reg, Xt` form.

## Exception And Breakpoint Instructions

The old surface encoded immediate exception instructions by packing a 16-bit
immediate into bits `[20:5]`:

```text
SVC: 0xd4000001 | (imm16 << 5)
HVC: 0xd4000002 | (imm16 << 5)
SMC: 0xd4000003 | (imm16 << 5)
BRK: 0xd4200000 | (imm16 << 5)
```

The implementation masked the immediate to 16 bits rather than performing
strict signedness or range diagnostics.

## SYS-Family String Encoders

Several system operations were encoded from raw operand text:

- `encode_ic` recognized `ialluis`, `iallu`, and `ivau`
- `encode_at` recognized `s1e1r`, `s1e1w`, `s1e0r`, and `s1e0w`
- `encode_tlbi` recognized a larger TLBI operation table
- `encode_sys` accepted the generic `sys #op1, Cn, Cm, #op2, Xt` form

`IC`, `AT`, and `TLBI` used precomputed base words and replaced only the low
five Rt bits. When the register operand was omitted, they used register 31.

`encode_sys` packed the fields directly:

```text
0xd5080000 | ((op1 & 7) << 16) | ((CRn & 0xf) << 12) |
((CRm & 0xf) << 8) | ((op2 & 7) << 5) | Rt
```

The generic parser accepted `C`/`c` prefixes for CRn and CRm and stripped a
leading `#` from op1/op2.

## TLB Maintenance Coverage

The TLBI table covered common ARMv8.0 operations:

```text
vmalle1is, vmalle1, alle1is, alle1, alle2is
vale1is, vale1, vale2is, vale2
vaae1is, vaae1, vaale1is, vaale1
vae1is, vae1, vae2is, vae2
aside1is, aside1
vmalls12e1is, vmalls12e1
ipas2e1is, ipas2e1, ipas2le1is, ipas2le1
```

It also recorded FEAT_TLBIRANGE-style names:

```text
rvae1is, rvale1is, rvaae1is, rvaale1is
rvae1, rvale1, rvaae1, rvaale1
rvae1os, rvale1os, rvaae1os, rvaale1os
ripas2e1is, ripas2e1, ripas2e1os
ripas2le1is, ripas2le1, ripas2le1os
```

The old table had a duplicate `rvaae1is` entry. A future implementation should
derive these encodings from named operation fields or validate the table
against the Arm ARM instead of preserving duplicates.

## Hints, BTI, And Cache Maintenance

`encode_bti` mapped raw suffixes to fixed HINT words:

```text
bti      -> 0xd503241f
bti c    -> 0xd503245f
bti j    -> 0xd503249f
bti jc   -> 0xd50324df
```

`encode_hint` packed a numeric HINT immediate as `CRm = imm >> 3` and
`op2 = imm & 7`, using base `0xd503201f`.

`encode_dc` recognized cache maintenance variants by substring matching the
operation text:

- `civac` -> `0xd50b7e20 | Rt`
- `cvac` -> `0xd50b7a20 | Rt`
- `cvap` -> `0xd50b7c20 | Rt`
- `cvau` -> `0xd50b7b20 | Rt`
- `ivac` -> `0xd5087620 | Rt`
- `zva` -> `0xd50b7420 | Rt`

The register operand was taken from the second parsed operand, or from the
last parsed register operand as a fallback. If no register was found, the old
code used `Rt = 0`.

## Hidden Assumptions And Rebuild Risks

- The old source used string tables for architectural system registers and
  maintenance operations. These tables need authoritative validation before
  becoming live assembler behavior.
- MRS and MSR have different read/write legality. The old implementation
  encoded many names but did not model permission, privilege level, feature, or
  access constraints.
- Generic system-register parsing accepted numeric fields after masking in
  `sysreg_encoding`; it did not reject out-of-range fields before packing.
- Exception, HINT, and PSTATE immediates were masked instead of being
  range-checked.
- Raw-string operand parsing was ad hoc and bypassed typed operand validation.
  Rebuilds should centralize parsing for `IC`, `AT`, `TLBI`, `SYS`, and `DC`
  operations.
- `encode_dc` used substring matching, which can select an operation even when
  the operation text contains extra unsupported spelling.
- Several base words were recorded as values copied from GCC/objdump comments.
  Revalidate them against architectural encoding tables before enabling them.
- This artifact is reconstruction guidance only. It is not proof that the
  current AArch64 assembler path supports system instructions.

## Reconstruction Notes

- Prefer typed enums for barrier domains, system-register names, cache
  operations, TLB operations, and address-translation operations instead of raw
  string matching.
- Keep MRS/MSR register tables generated or table-driven with explicit
  read/write capability metadata.
- Share one `SYS`-family encoder once the mnemonic-specific operation has been
  resolved to `op1`, `CRn`, `CRm`, `op2`, and `Rt`.
- Preserve generic `s<op0>_<op1>_c<CRn>_c<CRm>_<op2>` support, but validate
  field ranges before packing.
- Add range diagnostics for immediates instead of silently masking exception,
  HINT, and PSTATE fields.
- For backend-owned output, system-operation facts should arrive as structured
  machine instruction nodes or lower encoding records, not raw strings
  recovered from printed `.s` text.
