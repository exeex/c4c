# Narrow RV64 Explicit-Register Syntax Decision

## Decision

One narrow syntax extension is valid without broad parser or inline-assembly
redesign: accept canonical RV64 GPR operand tokens `{xN}`, `={xN}`, and
`+{xN}` for `0 <= N <= 31`, classify them in LIR-to-BIR using the active target
profile, and publish structured explicit-register facts on
`InlineAsmOperandMetadata`.

This is a new semantic BIR token family, not preservation of a previously
supported family. It should be implemented in a separate syntax-capability idea
before idea 724 resumes. The source parser, HIR, HIR-to-LIR lowering, LIR
storage, and LIR verifier already transport these brace groups; they do not need
new target grammar. Regalloc admission/enforcement remains out of scope for
this syntax slice.

## Ownership comparison

### Source parser / HIR

`src/frontend/hir/impl/stmt/stmt.cpp` intentionally retains quoted operand
constraint text and keeps clobbers separate. `InlineAsmStmt` in
`src/frontend/hir/hir_ir.hpp` is a target-neutral raw semantic carrier.

Adding RV64 parsing here would duplicate brace/token logic before HIR-to-LIR
role normalization, require target-profile access in frontend lowering, and
still need BIR classification. Keep source/HIR as transport and expression-role
ownership.

### HIR-to-LIR

`src/codegen/lir/hir_to_lir/stmt.cpp` already handles brace-aware splitting,
output/read-write normalization, ties, argument ordering, and separate clobber
rendering. It preserves `{xN}` bodies and produces `={xN}` for scalar outputs.

This layer owns role rendering but should not own target legality. Teaching it
RV64 register numbers would mix LLVM-compatible textual rendering with semantic
BIR admission and duplicate later validation.

### LIR and verifier

`LirInlineAsmOp` in `src/codegen/lir/ir.hpp` is a raw textual call carrier.
`src/codegen/lir/verify.cpp` validates result/type shape and optional `.insn r`
operand indices, not the complete constraint language.

The verifier should continue accepting transportable text. Rejecting `{xN}`
there would require duplicating target-aware operand-role parsing and would
erase the precise BIR unsupported-fact surface. It may retain generic structural
checks only.

### Semantic BIR classifier

`src/backend/bir/lir_to_bir/calling.cpp::make_inline_asm_metadata` already owns
the exhaustive accepted token grammar, argument/output/tie indexing,
register-class/group facts, unsupported-fact vocabulary, and the conversion to
`InlineAsmOperandMetadata`. Its caller `lower_inline_asm_call` has access to
`context_.target_profile`.

This is the correct owner. Extend `make_inline_asm_metadata` to receive the
target profile and classify only the bounded RV64 family. Other targets and
other brace spellings stay unsupported.

## Bounded syntax

Accepted only when `target_profile.arch == TargetArch::Riscv64`:

| Token | Role | Indices | Register fact |
|---|---|---|---|
| `{xN}` | `RegisterInput` | next argument index | explicit RV64 GPR physical index N |
| `={xN}` | `RegisterOutput` | next output index | explicit RV64 GPR physical index N |
| `+{xN}` | `RegisterOutput` | next argument and output indices | explicit RV64 GPR physical index N |

`N` is canonical ASCII decimal with no sign, whitespace, leading zero for
multi-digit values, alias spelling, suffix, alternative, or modifier. The
initial family is `x0` through `x31` only. ABI aliases (`a0`, `t0`, `s1`), FPRs,
vector registers, alternatives, and multiple-register groups remain
unsupported even though later code can recognize some aliases during home
validation.

Restricting syntax to canonical `xN` separates semantic support from alias
normalization and makes malformed behavior deterministic. Whether particular
identities such as `x0`, stack pointer, or reserved platform registers are
legal for an input/output value is a prepared target-legality decision; syntax
classification preserves the identity but does not make it allocatable.

## Structured BIR metadata

Add a BIR-owned fact equivalent to:

```text
InlineAsmExplicitRegisterConstraint
  bank: General
  physical_index: uint32
  canonical_spelling: string
```

and an optional `explicit_register` field on `InlineAsmOperandMetadata`.

Invariants:

- the field is present only for `RegisterInput` or `RegisterOutput` rows;
- `register_class` is `General` and `register_group_width` is one;
- physical index is parsed once from the canonical token;
- `constraint`, role indices, and explicit fact describe the same token;
- clobber rows never carry the field;
- no `PreparedTargetRegisterIdentity` dependency is introduced into BIR.

The prepared layer can later combine the active target architecture with bank
and physical index to form `PreparedTargetRegisterIdentity`. Raw spelling is a
diagnostic/canonicalization check, not allocation authority by itself.

## Classification and malformed-input behavior

The classifier should use one helper that strips exactly one allowed role
prefix, recognizes the complete `{xN}` body, and returns either a structured
fact or a precise reason. It must not use substring matching.

Required outcomes:

| Input | Result |
|---|---|
| `{x10}`, `={x10}`, `+{x10}` on RV64 | supported role plus structured GPR index 10 |
| same tokens on x86/AArch64/unknown target | `unsupported_explicit_register_target<index>:<token>` |
| `{}`, `{x}`, `{x-1}`, `{x32}`, `{x999}` | `malformed_explicit_register_constraint<index>:<token>` |
| `{x01}` | malformed non-canonical spelling |
| `{a0}`, `{t0}`, `{f0}`, `{v0}` | `unsupported_explicit_register_constraint<index>:<token>` in the initial family |
| `=={x10}`, `++{x10}`, `&{x10}`, `{x10}|r`, whitespace variants | malformed/unsupported; never silently normalized |
| `~{x10}` with matching clobber vector | existing `Clobber`; no explicit value-register fact |
| `~{x10}` without matching vector entry | existing `unsupported_clobber_constraint<index>` |

The generic `unsupported_constraint` fallback remains for tokens that are not
explicit-register-looking. Explicit-looking malformed forms receive the narrow
typed vocabulary above so tests can distinguish syntax, target, and clobber
failures without weakening admission.

## Deterministic semantic proof

The syntax implementation requires proof at three boundaries, stopping before
regalloc:

1. **Source-to-LIR positive:** an RV64 source case with one input `{x10}`, one
   scalar output `={x11}`, and one read/write `+{x12}` shows HIR-to-LIR retains
   the brace bodies, role prefixes, operand ordering, and separate clobbers.
   Each constraint has independent inline-assembly operand meaning; no phi or
   publication assertion appears in this test.
2. **LIR-to-BIR positive:** semantic lowering produces the expected
   `RegisterInput`/`RegisterOutput` roles, indices, general class/width, and
   structured physical indices 10/11/12 with no unsupported facts.
3. **Negative table:** focused tests cover wrong target, empty/nondecimal/range/
   canonicality failures, aliases, repeated prefixes, and structured versus
   rendered-only clobbers with exact unsupported facts.

Determinism comes from source tokens and parsed indices, not allocator order.
Reordering unrelated instructions or changing register pressure cannot affect
the BIR metadata assertions. Tests must not manually construct the positive
`InlineAsmOperandMetadata`; a direct LIR fixture may supplement source coverage
for malformed cases that the frontend rejects, but cannot replace the positive
source route.

## Implementation boundary

A separate implementation idea may touch only:

- `InlineAsmOperandMetadata` and a small BIR-owned explicit-register fact;
- `make_inline_asm_metadata` plus its target-profile call site;
- source semantic-BIR and LIR-to-BIR focused tests for the bounded RV64 tokens.

It must not touch prepared constraint production, regalloc, value homes,
publication queries, or target emission. After that syntax idea closes, idea
724 may resume at prepared ingress using the structured BIR fact.

## Stop and reject conditions

Stop rather than implement if the positive source constraint is discarded or
rewritten before `LirInlineAsmOp`, if accepting it requires changing general
GCC parsing rules, or if target profile cannot be supplied to the BIR
classifier without broad interface churn.

Reject any route that repurposes `~{xN}`, accepts arbitrary brace contents,
stores only an unvalidated raw name, injects positive BIR facts in a fixture, or
enters regalloc/publication work. Those routes would either invert clobber
semantics or hide the original discontinuity rather than add a bounded syntax.

## Completion statement

Narrow support is valid. Semantic BIR classification is the sole grammar owner;
source/HIR/LIR remain transport and role-rendering layers. The accepted family
is canonical RV64 GPR `{xN}` input/output/read-write syntax with structured BIR
bank/index metadata and precise malformed/unsupported facts. Implementation
must be a separate syntax-only idea before idea 724 resumes.
