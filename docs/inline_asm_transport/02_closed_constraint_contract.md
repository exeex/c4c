# Closed RV64 Inline-Assembly Constraint Contract

## Authority and target scope

This first contract is RV64-only. The target-independent schema can represent
other targets later, but `TargetArch` values other than `Riscv64` must receive
`UnsupportedInlineAsmTarget` until they have their own reviewed table.

The admitted vector group set is `VR`, `VRM2`, `VRM4`, and `VRM8`:

| Class token | Class | Width | Required base alignment | Occupied units |
| --- | --- | ---: | ---: | --- |
| `VR` | vector | 1 | 1 | `vN` |
| `VRM2` | vector | 2 | 2 | `vN..vN+1` |
| `VRM4` | vector | 4 | 4 | `vN..vN+3` |
| `VRM8` | vector | 8 | 8 | `vN..vN+7` |

The group is atomic and contiguous, `N % width == 0`, and `N + width <= 32`.
The [LLVM RISC-V vector guide](https://llvm.org/docs/RISCV/RISCVVectorExtension.html)
lists exactly these four register classes and their legal bases. The
[ratified RISC-V V specification](https://docs.riscv.org/reference/isa/unpriv/v-st-ext)
independently requires integer LMUL 1/2/4/8, consecutive registers, and bases
aligned to the group width. Active `src/codegen/lir/types.hpp::LirTypeRef` and
`src/frontend/parser/impl/types/types_helpers.hpp::match_c4c_builtin_vrm_type`
also recognize widths 1/2/4/8.

The quarantined classifier accepted `VRM1` as an alias for `VR`, but LLVM does
not publish a `VRM1` class. `VRM1` is therefore explicitly rejected as
`UnsupportedConstraintClass`, not guessed to be an alias.

## Accepted operand tokens

Whitespace, alternatives, combined classes, implicit defaults, and case
folding are not permitted. `CLASS` below is exactly one of `r`, `VR`, `VRM2`,
`VRM4`, or `VRM8`.

| Source token | Source position | Normalized role | Read | Write | Early clobber | Tie |
| --- | --- | --- | --- | --- | --- | --- |
| `CLASS` | input | `Use` | yes | no | no | none |
| `=CLASS` | output | `Def` | no | yes | no | none |
| `+CLASS` | output | `UseDef` | yes | yes | no | self |
| `=&CLASS` | output | `Def` | no | yes | yes | none |
| `+&CLASS` | output | `UseDef` | yes | yes | yes | self |
| canonical decimal `N` | input | `TiedUse` | yes | through output `N` | inherited from output | output operand `N` |

`r` means one allocatable RV64 GPR. It does not mean any physical register:
the target register-info service removes architectural, ABI, frame, and
backend-reserved registers before allocation. No fallback from vector to `r`
or from an unsupported width to width one is allowed.

Numeric ties use zero-based source output ordinals, must refer to a preceding
register output, and permit at most one explicit tied input per output. The
input and output must have identical class, group width, and value-compatible
type. A `+CLASS` source output normalizes directly to one `UseDef` virtual
operand; LLVM compatibility rendering may show `=CLASS,N`, but that rendering
is not semantic input.

`&` is accepted only in the exact output prefixes above. An early-clobber
assignment must not overlap any non-tied input live at the asm. A tied input may
share its own early-clobber output, while every other input is excluded. This
matches the [LLVM inline-asm constraint contract](https://www.llvm.org/docs/LangRef.html#inline-assembler-expressions).

## Accepted clobbers

Clobbers come only from the source clobber vector; rendered LLVM `~{...}` text
is never semantic authority.

| Source clobber | Normalized fact | Allocation effect |
| --- | --- | --- |
| `memory` | `MemoryClobber` | side-effect/scheduling barrier; no register candidate |
| `cc` | `ConditionCodeClobber` | target condition-code effect; no GPR/vector candidate |
| canonical `xN`, `0 <= N <= 31` | physical GPR unit | exclude `xN` from every overlapping asm operand assignment |
| canonical `vN`, `0 <= N <= 31` | physical vector unit | exclude every candidate group containing `vN` |

Register aliases (`zero`, `ra`, `sp`, `a0`, `t0`, and similar) are rejected in
this first table. Canonical index spelling prevents two strings from denoting
one physical unit. An eventual operand assignment may not overlap an explicit
register clobber, but this is allocation-owned because the admitted grammar has
no fixed-register operand. Duplicate identical clobbers are rejected rather
than silently deduplicated.

## Template references and modifiers

Template bytes are not validated before late assembly. At the late seam, the
closed substitution grammar is:

| Form | Late result |
| --- | --- |
| `%%` | literal `%` |
| `%N` | assigned source operand `N`, using its allocated base register |
| `%zN` | `zero` for an admitted immediate zero, otherwise normal operand spelling |
| `%iN` | `i` for an admitted immediate, otherwise empty |

This checkpoint does not yet admit immediate constraint classes, so `%zN` and
`%iN` are representable late-assembler forms but fail
`TemplateModifierOperandMismatch` for the current `r`/vector-only operand set.
GCC documents `z` and `i` as the RV64 target modifiers in its
[RISC-V operand modifier table](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#RISC-V-Operand-Modifiers).

Named `%[name]` references are `UnsupportedNamedOperand` because the active
parser discards source operand names. `${...}` is LLVM compatibility syntax
and is never accepted in the opaque source payload. Every other alphabetic
modifier is `UnsupportedTemplateModifier`. These diagnostics occur only at
late assembly, never during LIR/BIR/MIR transport or allocation.

## Stable rejection categories

Normalization at LIR-to-BIR returns one category plus operand/clobber index and
original spelling:

| Category | Condition |
| --- | --- |
| `UnknownInlineAsmTarget` | `LirModule::target_profile.arch` is `Unknown` |
| `UnsupportedInlineAsmTarget` | source target is known but not RV64 |
| `EmptyConstraint` | missing/empty source token |
| `InvalidConstraintRole` | input has output prefix, output lacks one, or `&` is misplaced |
| `UnsupportedConstraintClass` | class is not the closed set, including `VRM1`, `VRM3`, memory, immediate, alternatives, or combined classes |
| `MalformedTie` | noncanonical decimal, out-of-range output, forward/non-output tie, or second tie |
| `ConstraintTypeMismatch` | scalar/vector class or group width disagrees with value type |
| `DuplicateClobber` | repeated structured clobber |
| `MalformedRegisterClobber` | noncanonical/out-of-range `xN` or `vN` |
| `UnsupportedClobber` | name outside `memory`, `cc`, canonical `xN`, canonical `vN` |

Alternatives (`|`), sets (`rm`), indirect `*`, commutative `%`, explicit fixed
operand registers (`{xN}`), memory/immediate classes, and implicit fallback
constraints are outside this checkpoint. Each fails closed; none is reduced to
`r` or width one.

Allocator and late failures are likewise stable:

- `IllegalRegisterGroup`, `MisalignedRegisterGroup`, and
  `NoncontiguousRegisterGroup` for malformed candidate/assignment facts;
- `TieRequirementConflict`, `EarlyClobberConflict`, `ClobberConflict`, and
  `NoLegalRegisterAssignment` for unsatisfiable allocation;
- `OperandReferenceOutOfRange`, `UnsupportedNamedOperand`,
  `UnsupportedTemplateModifier`, `TemplateModifierOperandMismatch`,
  `AsmParseError`, and `AsmEncodingError` at late assembly.

`OperandClobberConflict` is deliberately absent from normalization. There is
no admitted fixed-register operand that could make it decidable there.
`ClobberConflict` remains the allocator-owned diagnostic when every otherwise
legal assignment overlaps a structured physical clobber.

## Historical conflict record

Reference-only `src/backend/bir/lir_to_bir/calling.cpp` accepted `VRM1` and
performed early `.insn`/template inspection. Reference-only
`src/backend/legacy/prealloc/target_register_profile.cpp` formed aligned
contiguous groups by stepping bases by width. The group behavior agrees with
the target specifications, but those files remain uncompiled and supply no
authority to restore their APIs or parsing route.
