# RV64 Explicit-Register Inline-Assembly Syntax Research

## Research set

1. [Source-to-BIR syntax flow](01_source_to_bir_syntax_flow.md) traces source
   constraints and clobbers through HIR, HIR-to-LIR role rendering,
   `LirInlineAsmOp`, shape-only LIR verification, and the closed semantic BIR
   classifier. It records accepted/rejected tokens and identifies
   `make_inline_asm_metadata` as the first explicit-register rejection boundary.
2. [Narrow support decision](02_narrow_support_decision.md) compares source/HIR,
   HIR-to-LIR, LIR/verifier, and semantic BIR ownership. It selects a bounded
   semantic BIR grammar for canonical RV64 GPR input/output/read-write tokens,
   structured metadata, precise malformed behavior, source-backed proof, and a
   syntax-only implementation boundary.

## Current flow conclusion

Raw physical operand text is already transported far enough:

- the source parser copies quoted operand constraints into `InlineAsmStmt` and
  stores clobber names separately;
- HIR-to-LIR preserves brace bodies, renders output/read-write roles, ties, and
  `~{name}` clobbers;
- `LirInlineAsmOp` stores raw constraint text plus a distinct clobber vector;
- the LIR verifier checks result/type shape and `.insn r` indices, not target
  constraint grammar;
- `make_inline_asm_metadata` first assigns structured operand meaning and sends
  `{xN}` forms to `unsupported_constraint`.

The missing capability is therefore semantic classification and BIR metadata,
not string transport. The later RV64 register-name parser validates
already-assigned homes and cannot serve as evidence of pre-regalloc ingress.

Clobbers remain a separate machine-state declaration. `~{xN}` says assembly may
alter xN; it does not bind an input/output named value to xN. BIR correctly
requires the rendered clobber token to agree with the structured clobber vector.

## Narrow syntax decision

The research supports one syntax-only implementation idea:

- on RV64, admit canonical `{xN}`, `={xN}`, and `+{xN}` for decimal N in
  0 through 31;
- classify them in `make_inline_asm_metadata`, whose caller already has the
  active target profile;
- publish a BIR-owned explicit-register fact containing general bank, physical
  index, and canonical spelling on the corresponding input/output operand row;
- retain current role indices, class `General`, and group width one;
- reject wrong targets, malformed braces/prefixes/ranges/canonicality, aliases,
  other banks, and unsupported alternatives with precise facts;
- leave prepared target legality, constraint admission, and allocator
  enforcement for the separately parked idea 724.

Source/HIR remain raw semantic transport, HIR-to-LIR remains role/text
rendering, and LIR verification remains structural. This avoids duplicated
target grammar and keeps semantic BIR as the sole accepted-token owner.

## Implementation boundary and stop conditions

The syntax implementation may touch only the small BIR metadata fact,
`make_inline_asm_metadata` and its target-profile call site, plus source-backed
semantic-BIR and focused malformed-token tests. It must not enter prepared
constraint production, regalloc, homes, publication, or target emission.

Stop if source/HIR-to-LIR does not preserve a positive brace token in practice,
if target profile cannot reach the classifier without broad interface churn,
or if support requires changing general GCC parsing rules. Do not substitute a
fixture-created positive BIR row.

## Cross-document consistency review

| Check | Result |
|---|---|
| First discontinuity | Consistent: answer 01 locates BIR classification; answer 02 assigns new grammar to that exact owner. |
| Source/LIR preservation | Consistent: answer 01 proves brace-aware transport; answer 02 requires a source-backed positive before accepting implementation. |
| Clobbers | Consistent: both answers keep the structured clobber vector and `~{name}` semantics separate from value operands. |
| Target validation | Consistent: BIR grammar uses active RV64 profile and publishes bank/index; prepared legality remains later and out of scope. |
| Scope | Consistent: both answers stop before prepared/regalloc/publication/emission work and avoid multi-target expansion. |
| Malformed behavior | Consistent: generic current rejection is refined only for the bounded explicit-looking family; unrelated fallback remains intact. |

No contradiction was found.

## Acceptance and reject-signal review

- Exact output: this directory contains `index.md` plus exactly the two required
  numbered answers.
- Concrete evidence: answer 01 cites source cases, HIR/LIR/BIR types, lowering,
  verifier, classifier, and focused tests.
- Narrow owner: answer 02 chooses semantic BIR rather than inferring support
  solely from post-regalloc home validation.
- Clobber integrity: neither answer repurposes `~{xN}` as value allocation.
- No testcase shaping: no fixture flags, named-value mapping, allocator
  pressure, or manually injected positive metadata is proposed.
- No expectation-only progress: unsupported relabeling, helper renames, and
  classification-only claims without a source positive are explicitly rejected.
- No broad expansion: general GCC/LLVM syntax, multi-target, ABI, regalloc,
  publication, and emission changes remain outside the implementation boundary.
- Original discontinuity is repaired at its owner rather than hidden behind a
  later field or parser.

The research set satisfies idea 725's documentation acceptance criteria and
triggers none of its reviewer reject signals.
