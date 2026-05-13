# AArch64 Encoder Load Store Legacy Surface

This artifact preserves the useful structure from the removed
`load_store.cpp` translation unit. The old file was a commented C++ mirror of
the reference Rust encoder surface at
`ref/claudes-c-compiler/src/backend/arm/assembler/encoder/load_store.rs`.

## Role

The removed surface documented how the legacy assembler encoder handled
AArch64 load/store addressing forms, pair loads/stores, exclusive and
acquire/release memory operations, address computation, prefetch operations,
and a subset of LSE atomic operations.

It did not provide active compiled C++ behavior in this tree; every routine was
preserved as commented Rust-shaped pseudocode. The surface expected operands
from the legacy assembler parser and returned either one instruction word or a
word paired with relocation metadata.

## Entry Points

The old surface listed these encoder entry points:

- `encode_ldr_str_auto(operands, is_load)`: inferred load/store width from the
  first register operand before delegating to `encode_ldr_str`
- `encode_ldr_str(operands, is_load, size, is_signed, is_128bit)`: shared
  scalar/FP/SIMD LDR and STR encoder for unsigned, unscaled, indexed,
  register-offset, literal, and relocation-bearing memory operands
- `encode_ldur_stur(operands, is_load, op2_bits)`: unscaled LDUR/STUR forms
  with register-class-derived size and opcode fields
- `encode_ldtr_sized(operands, is_load, size)`: explicitly sized unprivileged
  LDTR/STTR byte, halfword, word, or doubleword forms
- `encode_ldrsw(operands)`: sign-extending word load into an X register,
  including immediate, pre/post-indexed, and register-offset forms
- `encode_ldrs(operands, size)`: LDRSB/LDRSH sign-extending byte and halfword
  loads into W or X destinations
- `encode_ldp_stp(operands, is_load)`: pair load/store forms for scalar and
  FP/SIMD registers
- `encode_ldnp_stnp(operands, is_load)`: non-temporal pair load/store forms
- `encode_ldxr_stxr(operands, is_load, forced_size)`: exclusive load/store,
  including byte and halfword variants
- `encode_ldaxr_stlxr(operands, is_load, forced_size)`: acquire/release
  exclusive load/store variants
- `encode_ldxp_stxp(operands, is_load, acquire_release)`: exclusive pair
  load/store and acquire/release pair variants
- `encode_ldar_stlr(operands, is_load, forced_size)`: acquire load and release
  store forms
- `encode_adrp(operands)` and `encode_adr(operands)`: page-relative and
  PC-relative address computation with relocatable symbol support
- `encode_prfm(operands)` and `encode_prfop(name)`: prefetch memory encoding
  and prefetch-operation-name mapping
- `encode_cas(mnemonic, operands)`: CAS/CASA/CASAL/CASL and byte/halfword
  compare-and-swap variants
- `encode_swp(mnemonic, operands)`: SWP and acquire/release byte/halfword swap
  variants
- `encode_ldop(mnemonic, operands)`: LDADD, LDCLR, LDEOR, LDSET and their
  acquire/release byte/halfword variants
- `encode_stop(mnemonic, operands)`: STADD, STCLR, STEOR, STSET aliases using
  the corresponding LD* atomic operation with `Rt = XZR/WZR`

## Single Load Store Forms

`encode_ldr_str_auto` inferred the data size from the first register spelling:

- `w*` selected 32-bit integer load/store
- `x*`, `sp`, `xzr`, and `lr` selected 64-bit integer load/store
- `s*` and `d*` selected 32-bit and 64-bit FP/SIMD load/store
- `q*` selected the 128-bit vector path, using the special load/store opcode
  adjustment for Q registers

`encode_ldr_str` then handled the common operand forms. Memory operands with a
non-negative aligned offset used the unsigned-offset form when the scaled
offset fit in the 12-bit field. Other immediate offsets fell back to the
unscaled LDUR/STUR-style form by masking the signed 9-bit immediate field.

Pre-indexed and post-indexed operands packed the same signed 9-bit immediate
field and selected the writeback mode through the fixed low option bits.
Register-offset operands accepted `lsl`, `sxtw`, `sxtx`, `uxtw`, `uxtx`, or no
extend marker. Missing extend defaulted to UXTW for W index registers and LSL
for X index registers.

The same routine also recognized two symbolic forms:

- memory register-offset operands whose index text started with `:lo12:` or
  `:got_lo12:` produced a load/store word with relocation metadata
- `LDR Rt, symbol` emitted the literal-load base word with an `Ldr19`
  relocation

The old code used the V bit for FP/SIMD registers and selected the opcode from
load/store direction, sign-extension, and the 128-bit Q-register mode.

## Signed And Unscaled Loads

`encode_ldur_stur` handled unscaled load/store instructions and selected
register size from the target register spelling. It treated Q, D, S, H, and B
FP/SIMD register prefixes separately from GP W/X registers.

`encode_ldtr_sized` encoded explicitly sized unprivileged transfer
instructions. It accepted only a memory operand and packed the delegated size
field, the load/store opcode, a signed 9-bit immediate, and the fixed
unprivileged op2 bits.

`encode_ldrsw` documented LDRSW coverage for:

- unsigned-offset memory operands when the offset was non-negative, 4-byte
  aligned, and fit in the scaled 12-bit field
- unscaled memory operands for other immediate offsets
- pre-indexed and post-indexed forms
- register-offset forms with explicit `lsl`, `sxtw`, `uxtw`, or `sxtx`
  combinations

`encode_ldrs` documented the LDRSB/LDRSH family. It selected the sign-extension
opcode from whether the destination was W or X, reused the scaled unsigned
offset path when possible, and supported unscaled, pre-indexed, post-indexed,
and register-offset addressing.

## Pair And Non-Temporal Pair Forms

`encode_ldp_stp` handled pair loads/stores with three operands:

```text
Rt1, Rt2, address
```

It selected the `opc` and scaling shift from whether the pair was integer,
single/double-precision FP, or Q-register SIMD. It supported signed-offset,
pre-indexed, and post-indexed pair addressing, each using a scaled signed
7-bit immediate.

`encode_ldnp_stnp` encoded non-temporal pair operations for integer registers
only. The old comments explicitly noted that FP/SIMD support with `V = 1` was
still missing. It accepted the signed-offset memory form and derived the scale
from W versus X register width.

## Exclusive And Acquire Release Forms

The removed surface grouped exclusive and acquire/release instructions in a
small set of helpers:

- `encode_ldxr_stxr` handled LDXR/STXR and byte/halfword variants, selecting
  size from `forced_size` or the data register width
- `encode_ldaxr_stlxr` added the acquire/release bit to the same single-register
  exclusive shape
- `encode_ldxp_stxp` handled LDXP, LDAXP, STXP, and STLXP pair forms, with
  load forms using the all-ones status/source register field and store forms
  consuming the leading W status register
- `encode_ldar_stlr` encoded LDAR/STLR and byte/halfword variants with the
  acquire/load bit selected from direction

These helpers accepted only simple `[Xn]` memory operands. The old surface did
not document indexed or offset forms for exclusive/acquire-release operations.

## Address Computation And Relocations

`encode_adrp` accepted symbols, symbol offsets, labels, and `:got:` modifiers.
It also treated parser-classified register, condition, and barrier operands as
symbols because names such as `s1`, `v0`, `d1`, `cc`, `lt`, `st`, and `ld` can
collide with other operand categories even though ADRP expects a symbol.

Relocation-bearing address forms were:

- `ADRP symbol` produced `RelocType::AdrpPage21`
- `ADRP :got:symbol` produced `RelocType::AdrGotPage21`
- `ADR symbol` produced `RelocType::AdrPrelLo21`
- `ADR #imm` packed the 21-bit immediate directly without relocation

The old `ADR #imm` path had a TODO for signed 21-bit range validation.

Load/store low-12 relocations were produced inside `encode_ldr_str`:

- `:lo12:` used one of `Ldst8AbsLo12`, `Ldst16AbsLo12`, `Ldst32AbsLo12`,
  `Ldst64AbsLo12`, or `Ldst128AbsLo12`
- `:got_lo12:` used `Ld64GotLo12`

The low-12 relocation selection depended on the element size and the Q-register
128-bit flag.

## Prefetch

`encode_prfm` accepted either a named prefetch operation or a numeric prefetch
operation in the first operand. Numeric forms had to fit the 5-bit operation
field.

The second operand supported:

- immediate memory addressing with non-negative 8-byte-aligned offsets fitting
  the scaled 12-bit field
- register-offset addressing with the same extend/shift family used by
  load/store register-offset forms

Symbol or label PRFM literal forms were explicitly recorded as unsupported.

`encode_prfop` mapped the legacy prefetch names:

```text
pldl1keep, pldl1strm, pldl2keep, pldl2strm, pldl3keep, pldl3strm
plil1keep, plil1strm, plil2keep, plil2strm, plil3keep, plil3strm
pstl1keep, pstl1strm, pstl2keep, pstl2strm, pstl3keep, pstl3strm
```

to their 5-bit encodings.

## LSE Atomic Families

The LSE atomic helpers decoded size and memory-order semantics from the
mnemonic suffix:

- `encode_cas` handled CAS/CASA/CASAL/CASL plus byte and halfword suffixes,
  deriving acquire from `a`, release from `l`, and size from `b`, `h`, or the
  compared register width
- `encode_swp` handled SWP/SWPA/SWPAL/SWPL with the same suffix policy
- `encode_ldop` handled LDADD, LDCLR, LDEOR, and LDSET, mapping the base
  mnemonic to the atomic operation field and suffixes to acquire, release, and
  size bits
- `encode_stop` handled STADD, STCLR, STEOR, and STSET as store aliases by
  encoding the matching LD* operation with the destination/result register set
  to register 31

All LSE atomic routines accepted simple three-operand or two-operand shapes
with a final `[Xn]` memory operand. The surface did not document offset or
indexed atomic memory operands.

## Dependencies And Assumptions

The removed surface depended on helper contracts from the old assembler,
encoder, and relocation modules:

- `Operand` from the assembler parser, including register, memory,
  pre-indexed memory, post-indexed memory, register-offset memory, symbol,
  symbol-offset, label, modifier, condition, and barrier forms
- `EncodeResult::Word` and `EncodeResult::WordWithReloc`
- `Relocation` and relocation kinds `Ldr19`, `AdrpPage21`, `AdrGotPage21`,
  `AdrPrelLo21`, `Ldst8AbsLo12`, `Ldst16AbsLo12`, `Ldst32AbsLo12`,
  `Ldst64AbsLo12`, `Ldst128AbsLo12`, and `Ld64GotLo12`
- `get_reg` for register number and GP width extraction
- `parse_reg_num` for base, index, pair, status, and atomic register fields
- `is_fp_reg` for selecting the FP/SIMD V bit
- `get_symbol` for symbolic ADR operands

The surface inferred register class and width primarily from register-name
prefixes. It frequently masked signed immediate fields rather than rejecting
out-of-range operands, and several register-offset helpers defaulted unknown
extend names to a valid encoding instead of producing a hard diagnostic.

## Failure Risks For Rebuild

- Single load/store dispatch mixes integer, FP/SIMD, signed, unsigned,
  register-offset, literal, and relocation-bearing forms. A rebuild should
  classify operand shape and transfer kind before packing fields.
- Q-register handling uses special size/opcode rules that differ between
  normal load/store and literal load forms; this should be centralized.
- Unsigned-offset versus unscaled fallback should include explicit signed range
  checks for the 9-bit unscaled field rather than masking arbitrary offsets.
- Register-offset addressing should validate allowed extend and shift amounts
  for the element size instead of treating any positive shift as the scale bit.
- LDRSW and LDRSB/LDRSH need strict destination-width validation so sign
  extension cannot silently encode the wrong `opc`.
- Pair operations should validate scaled signed 7-bit ranges and register
  class compatibility for both pair members.
- Exclusive and acquire/release helpers should reject invalid operand counts
  and enforce W status-register requirements for store-exclusive forms.
- ADR and ADRP relocations need clear parser policy for symbol names that look
  like registers, conditions, or barriers.
- PRFM should keep named operation parsing separate from immediate operation
  values, and a rebuild should decide whether symbolic PRFM literal references
  are supported or diagnosed.
- LSE atomic suffix parsing based on substring tests can confuse malformed
  mnemonics. A rebuilt path should parse base operation, width suffix, acquire,
  and release flags through an explicit mnemonic table.
- Low-12 load/store relocation selection must stay aligned with linker scaling
  and overflow behavior for 8-, 16-, 32-, 64-, and 128-bit transfers.

## Rebuild Guidance

Use this artifact as the historical map for load/store and memory-operation
encoder behavior. A rebuilt live encoder should split address-mode parsing,
transfer-width classification, relocation construction, and field packing into
small helpers so diagnostics and relocation scaling rules are visible at the
assembler/linker boundary.
