# RV64 Explicit-Register Inline-Assembly BIR Syntax

Status: Open
Type: Narrow semantic BIR syntax capability
Derived from: `ideas/closed/725_rv64_explicit_register_inline_asm_syntax_research.md`
Prerequisite for: `ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md`

## Goal

Admit canonical RV64 GPR `{xN}`, `={xN}`, and `+{xN}` inline-assembly operand
tokens into semantic BIR as structured input/output register facts, with
source-backed proof and precise malformed/unsupported behavior.

## Why This Exists

Source/HIR/LIR already preserve brace groups, but
`make_inline_asm_metadata` is the first closed grammar and currently sends all
physical operand tokens to `unsupported_constraint`. Research proved this can
be repaired narrowly at the BIR classifier without changing general source
parsing, LIR verification, prepared allocation, or target emission.

## In Scope

- Add a small BIR-owned explicit-register fact carrying general bank, physical
  index, and canonical spelling on `InlineAsmOperandMetadata`.
- Pass the active target profile into `make_inline_asm_metadata` from the
  existing `lower_inline_asm_call` context.
- On RV64 only, classify canonical `{xN}`, `={xN}`, and `+{xN}` for decimal
  N from 0 through 31 into existing input/output/read-write roles and indices.
- Preserve general register class and group width one.
- Add precise wrong-target, malformed brace/prefix/range/canonicality, alias,
  and unsupported-family facts while keeping unrelated generic fallback stable.
- Preserve structured clobber agreement and ensure `~{xN}` never receives a
  named-value explicit-register fact.
- Add a genuine source-to-LIR-to-BIR positive plus focused direct-LIR malformed
  negatives when the source parser cannot express malformed cases.

## Out Of Scope

- Prepared request production, target legality, regalloc constraint
  normalization/enforcement, value homes, move publication, or typed queries.
- AArch64/x86, FPR/vector/alias/multi-register/alternative syntax.
- General GCC/LLVM parser changes or LIR grammar validation.
- Arbitrary named-value register controls, fixtures as positive capability,
  target emission, ABI policy, or expectation downgrades.

## Required Invariants

- Semantic BIR is the sole target-aware accepted-token owner.
- Structured bank/index is authoritative; spelling is canonical diagnostic data.
- Role prefix, argument/output indices, class, width, and explicit fact agree.
- Only source-backed positive evidence counts as capability proof.
- Clobbers remain separate machine-state declarations.
- Unsupported tokens never become partially available structured authority.

## Acceptance Criteria

- RV64 source cases for input, scalar output, and read/write canonical x-register
  constraints reach BIR with exact roles/indices and structured physical indices.
- Wrong-target and malformed/unsupported forms publish precise fail-closed facts.
- Existing class, vector, tie, immediate, memory/address, and clobber behavior
  remains unchanged.
- No prepared/regalloc/publication/emission file changes occur.
- Matching focused before/after proof has no new failures beyond the accepted
  `backend_prealloc_inline_asm` baseline failure.

## Stop Condition

Stop for lifecycle review if the source positive is rewritten or discarded
before LIR, target profile cannot reach the classifier without broad interface
churn, or implementation requires general parser/LIR redesign.

## Reviewer Reject Signals

- Reject positive proof based only on manually constructed BIR metadata or the
  post-regalloc RV64 home parser.
- Reject clobber `~{xN}` treated as input/output allocation authority.
- Reject substring matching, arbitrary brace contents, noncanonical aliases,
  wrong-target acceptance, or raw spelling without bank/index validation.
- Reject fixture flags, named-value maps, allocator pressure, prepared homes,
  regalloc, publication, or target emission changes.
- Reject expectation downgrades, unsupported relabeling, helper renames, or
  diagnostics-only changes claimed as syntax support.
- Reject broad multi-target, GCC/LLVM grammar, ABI, or inline-assembly redesign.
- Reject retaining `unsupported_constraint` for the accepted canonical source
  family behind a new metadata field.
