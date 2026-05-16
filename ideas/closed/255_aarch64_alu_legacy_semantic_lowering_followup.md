# AArch64 ALU Legacy Semantic Lowering Follow-Up

Status: Closed
Created: 2026-05-16
Source: Closure follow-up from
`ideas/closed/251_aarch64_alu_markdown_shard_implementation_redistribution.md`

## Intent

Decide whether the legacy-only ALU lowering guidance from the deleted
`src/backend/mir/aarch64/codegen/alu.md` shard should become real structured
AArch64 MIR lowering behavior, and implement only the parts that are still
semantically valid for the current backend pipeline.

## Why This Exists

The ALU shard redistribution moved current live ALU construction, lowering,
and printer ownership into `alu.cpp` and `alu.hpp`. During closure, reviewer
and executor notes classified several old text-emitter routes as non-live
legacy guidance rather than current compiled behavior. Those notes should not
block closure of the ownership redistribution, but they are durable enough to
preserve as a separate future initiative because `todo.md` is removed when the
completed plan closes.

## In Scope

- Audit unary integer helpers, count-leading-zero, count-trailing-zero,
  byte-swap, popcount, unsigned power-of-two division/remainder reductions,
  accumulator fallback division/remainder/variable-shift behavior,
  register-direct scratch handling, 32-bit extension behavior, and i128 copy
  against the current structured MIR and allocation-result pipeline.
- Rebuild selected behavior through typed MIR records, allocation facts, and
  ALU-owned lowering or printing helpers where the behavior is still valid.
- Preserve unsigned power-of-two reductions only for unsigned division and
  remainder unless a signed equivalent is separately designed and proved.
- Model signed 32-bit extension and unsigned zero-extension as explicit
  post-operation behavior where such lowering is introduced.
- Add targeted proof for each restored or newly modeled semantic route.

## Out of Scope

- Reopening the completed ALU markdown redistribution route.
- Recreating the old text-emitter accumulator convention as the semantic
  carrier for modern AArch64 lowering.
- Redistributing other AArch64 markdown shards.
- Treating old commented Rust mirror behavior as automatically accepted
  current compiler behavior without current-pipeline design and proof.
- Broad AArch64 cleanup unrelated to these ALU semantic routes.

## Acceptance Criteria

- The follow-up explicitly decides which legacy routes are still valid and
  which are obsolete for the current backend.
- Any implemented route is represented through structured MIR records,
  allocation-result facts, and ALU-owned helpers rather than final assembly text
  shortcuts.
- Targeted backend tests or existing focused tests prove each accepted route,
  including relevant width, extension, scratch, and fallback behavior.
- Existing supported ALU paths are not weakened or marked unsupported.
- The route does not require restoring `alu.md`.

## Reviewer Reject Signals

Reject the route or slice if it:

- adds named-case shortcuts, testcase-shaped matching, or final-assembly text
  patterning instead of structured semantic lowering;
- downgrades test expectations, marks supported ALU paths unsupported, or
  weakens contracts without explicit user approval;
- claims capability progress through helper renames, classification-only
  changes, or expectation rewrites while the semantic route remains absent;
- revives the old accumulator/text-emitter convention as the primary semantic
  carrier instead of using current MIR records and allocation facts;
- treats signed division or remainder power-of-two behavior as equivalent to
  unsigned reduction without separate signed proof;
- omits proof for 32-bit signed extension, unsigned zero-extension, right-side
  destination-register conflicts, or i128 high-half preservation when those
  routes are in scope;
- broadens into other AArch64 shard redistribution work or unrelated backend
  cleanup.

## Closure Note

Closed after Step 6 acceptance review. The accepted semantic routes from this
follow-up were classified, implemented, and proved through structured AArch64
MIR records, allocation facts, and ALU-owned helpers. The close-time regression
guard passed against the canonical `test_before.log` and `test_after.log`
pair.

Scratch/overlap/fallback authority work and popcount-style temporary authority
questions remain out of scope for this closed idea. They should not resume as
ALU legacy follow-up work unless a later lifecycle-reviewed idea first defines
prepared scratch and allocation authority, then proves a concrete backend
behavior consumes that authority.
