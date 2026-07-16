# Review A: Idea 839 Step 3 Function-Signature/Call-Composition Route

Active source idea: `ideas/open/839_lir_nominal_function_signature_call_composition.md`

Review base: `eae6db334` (`[plan] Activate function signature composition idea`). This is the active-idea activation checkpoint for idea 839; later lifecycle commit `1bb07681b` only advances `todo.md` to Step 3 and does not reset or materially change the source idea.

Commits reviewed: 11 commits from `eae6db334..HEAD`.

Worktree note: `todo.md` has one unstaged hook/reminder line. I did not treat that as committed lifecycle evidence.

## Findings

### Medium: template-origin compatibility repair uses retained signature text as a verifier escape hatch

The route mostly moves declaration/call authority into `LirFunctionSignatureRef` and the module signature store, but the recent compatibility repair lets `fn.signature_text.find("; template-origin:")` bypass mismatched aggregate signature mirror checks in `verify_function_signature_return_type_ref_mirror` and `verify_function_signature_param_type_ref_mirror` (`src/codegen/lir/verify.cpp:4190`, `src/codegen/lir/verify.cpp:4267`). This is not call-signature construction from spelling, and it appears intended as a named compatibility repair for templated aggregate ABI baselines. Still, it is a real debt marker because retained output text now controls a verifier leniency branch.

Impact: this does not require a route reset, but the next packet should not broaden this pattern. If template-origin compatibility remains necessary, it should be kept as a narrow named adapter or moved behind a structured fact before Step 5 deletion claims.

### Low: direct-call signature refs are intentionally partial, so Step 3 is not ready to close broadly

`direct_callee_signature_ref` only publishes a ref for resolved, non-extern, non-variadic, non-unspecified direct callees and also rejects byval signature entries (`src/codegen/lir/hir_to_lir/call/target.cpp:222`). That is consistent with the source idea's compatibility boundaries and with avoiding 829/830 argument identity claims, but it leaves byval, raw extern, no-prototype, variadic, and indirect call paths on retained compatibility structures.

Impact: safe to continue, but the next packet should name exactly one remaining consumer/shape. Do not mark Step 3 complete until fixed, variadic, aggregate parameter/return, malformed, and raw-call compatibility cases are covered as the runbook requires.

## Alignment Evidence

- `LirFunctionSignatureRef` and `LirCallOp.callee_signature_ref` are module refs, not rendered strings (`src/codegen/lir/ir.hpp:486`, `src/codegen/lir/ir.hpp:572`).
- The module store interns return refs, return extension attributes, parameter refs, byval facts, variadic state, and void-list state (`src/codegen/lir/ir.hpp:1424`).
- Lowering registers function signature refs from structured signature fields (`src/codegen/lir/hir_to_lir/hir_to_lir.cpp:673`) and publishes direct-call `callee_signature_ref` from the resolved callee's store entry only after retained structured facts agree (`src/codegen/lir/hir_to_lir/call/target.cpp:247`).
- The verifier rejects stale direct-call refs and requires agreement with the resolved callee store entry and call-site return facts (`src/codegen/lir/verify.cpp:554`).
- The printer renders function signatures and migrated direct-call suffixes from the store when refs are present, falling back to legacy text only when no ref is available (`src/codegen/lir/lir_printer.cpp:102`, `src/codegen/lir/lir_printer.cpp:162`, `src/codegen/lir/lir_printer.cpp:473`, `src/codegen/lir/lir_printer.cpp:670`).
- Tests cover store-backed direct-call refs, stale suffix ignoring, stale call ref rejection, missing-ref compatibility, raw direct construction compatibility, and signature-text printer drift for declarations/definitions (`tests/frontend/frontend_lir_call_type_ref_test.cpp:9238`, `tests/frontend/frontend_lir_call_type_ref_test.cpp:9463`, `tests/frontend/frontend_lir_call_type_ref_test.cpp:9473`, `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp:1231`, `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp:1358`).

## Judgments

Idea-alignment judgment: matches source idea.

Runbook-transcription judgment: plan matches idea.

Route-alignment judgment: on track.

Technical-debt judgment: watch.

Validation sufficiency: needs broader proof for Step 3 or later acceptance; current `test_after.log` is sufficient for the just-finished focused direct-call printer/ref slice plus shared backend checkpoint, but not for declaring the whole Step 3/idea acceptance complete.

Reviewer recommendation: narrow next packet.

## Recommendation

Continue the current route without plan/todo rewrite first. The next packet should be explicitly narrow: either migrate one remaining supported direct-call shape/consumer to `callee_signature_ref`, or contain the template-origin compatibility exception behind a better named/structured boundary. Do not expand the source idea, do not claim 829/830 argument value identity, and do not treat raw/byval/extern/no-prototype/variadic compatibility as completed merely because the fixed direct aggregate case is now store-backed.
