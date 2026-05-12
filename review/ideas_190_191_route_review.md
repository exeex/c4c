# Ideas 190/191 Route Review

## Scope

- Delegated review range: `fd94a9777` through `c4d3df769`.
- Diff base used for review: `8e13c6b86b6935010f149784e9fe5fb761e39f94`, the parent of `fd94a9777` and the lifecycle switch that activated idea 190.
- Commit count reviewed: 15 commits from `8e13c6b86..c4d3df769`.
- Source idea paths:
  - `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`
  - `ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md`

No active `plan.md` exists because both ideas are closed, so this review is historical and anchored on the explicit commit range plus the closed source ideas.

## Findings

No blocking findings.

### Low: Current canonical logs are not a single whole-range before/after pair

The current root `test_before.log` and `test_after.log` are both full-suite logs with `3137/3137` passing, but `test_before.log` records `Baseline Commit: bb3cdd54e140dcd0042d0920773e6839ceefb3fc`, which is inside the requested review range. That means the current canonical pair proves the final idea 191 close scope, not the entire combined range from before idea 190.

This is not a route blocker for continuing lifecycle work because:

- The focused tests added for ideas 190 and 191 directly exercise stale rendered-text rejection.
- `test_after.log` proves the current `HEAD` full suite is green.
- The closed ideas record close-time full-suite guard evidence.

If the supervisor needs an archival proof artifact for the whole combined range, regenerate a matching full-suite `test_before.log` at `8e13c6b86` and `test_after.log` at `c4d3df769`. That is an audit artifact gap, not implementation drift.

## Route Review

Idea 190 matches its source idea. `LirCallOp` gained a structured generated argument carrier in `src/codegen/lir/ir.hpp:286` and generated calls populate it from `OwnedLirTypedCallArg` in `src/codegen/lir/call_args_ops.hpp:64`. BIR call lowering gives non-empty `structured_args` first authority in `BirFunctionLowerer::parse_typed_call` at `src/backend/bir/lir_to_bir/calling.cpp:140`, and direct global call parsing routes through that path when a call has either a callee signature or structured args at `src/backend/bir/lir_to_bir/calling.cpp:349`.

The byval call layout path also uses structured argument type refs before legacy rendered text in `src/backend/bir/lir_to_bir/calling.cpp:706`. The remaining `args_str` and `callee_type_suffix` parsing is fenced to compatibility or validation/inference cases, not introduced as a new semantic authority over structured generated call arguments.

Idea 191 matches its source idea. `LirSignatureParam` now carries `is_byval` at `src/codegen/lir/ir.hpp:516`, HIR-to-LIR populates it from target ABI classification at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:253`, and verification rejects byval mirror/metadata disagreement at `src/codegen/lir/verify.cpp:823`. BIR signature parsing now carries `is_byval` through `ParsedFunctionSignatureParam` at `src/backend/bir/lir_to_bir/call_abi.cpp:283` and structured signature params source that field from metadata at `src/backend/bir/lir_to_bir/call_abi.cpp:310`.

The critical source-idea requirement is met in `collect_aggregate_params`: generated metadata-rich functions choose `structured_signature_params(function_)` instead of parsing `function_.signature_text` at `src/backend/bir/lir_to_bir/aggregate.cpp:186`, then use `parsed_param.is_byval` as the explicit byval authority at `src/backend/bir/lir_to_bir/aggregate.cpp:217`. Stale byval text without the metadata flag fails closed rather than silently recovering from `signature_text` at `src/backend/bir/lir_to_bir/aggregate.cpp:221`.

## Overfit And Downgrade Review

No testcase-overfit pattern found. The diff adds general metadata fields and routes existing lowering through those structured facts. I did not find new named-case dispatch, printed-LLVM substring probes, minimal backend shortcut emitters, supported-to-unsupported expectation rewrites, disabled tests, or narrowed test contracts in the reviewed test diff.

The tests are focused but not merely printer assertions:

- Direct scalar and direct byval stale `args_str` cases are built with stale rendered text and structured facts in `tests/backend/backend_lir_to_bir_notes_test.cpp:2329` and `tests/backend/backend_lir_to_bir_notes_test.cpp:2434`.
- Indirect stale suffix mismatch and fail-closed coverage is at `tests/backend/backend_lir_to_bir_notes_test.cpp:2645` and `tests/backend/backend_lir_to_bir_notes_test.cpp:2693`.
- No-signature structured intrinsic argument inference is covered at `tests/backend/backend_lir_to_bir_notes_test.cpp:2707`.
- Missing structured byval flag fails closed despite stale byval text at `tests/backend/backend_lir_to_bir_notes_test.cpp:2941`.
- Frontend producer coverage checks structured call args and byval signature metadata at `tests/frontend/frontend_lir_call_type_ref_test.cpp:434` and `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp:348`.
- Verifier disagreement coverage for byval mirror versus metadata is at `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp:495`.

## Judgments

- Idea alignment: `matches source idea`.
- Runbook transcription: `plan matches idea` for the historical runbooks; no active plan remains.
- Route alignment: `on track`.
- Technical debt: `acceptable`.
- Validation sufficiency: full acceptance evidence is sufficient for continuing lifecycle work, with the audit caveat above about the current canonical logs not being one whole-range proof pair.
- Reviewer recommendation: `continue current route`.
