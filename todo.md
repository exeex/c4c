# Current Packet

Status: Complete
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.19
Current Step Title: Receive the checked builtin-popcount call/narrow/final-use

## Just Finished

- Step 7.19 complete: received only producer-verified builtin-popcount `Ctpop`
  i32 results through one exact later i32 Add and i64 results through one exact
  i64-to-i32 Trunc then i32 Add. Raw-BIR builder and verifier admit the typed
  i32 Ctpop direct final use only when zero-count behavior is absent; importer
  validation keeps result, direct LinkNameId, one-integer signature, cast, and
  final-use linkage current-function structured authority.

## Suggested Next

- Send the exhausted runbook to plan-owner for an explicit source-completion,
  repair, replacement, or conclusion decision; do not infer source completion.

## Watchouts

- Ctpop accepts no `zero_count_behavior`; callee/result/argument displays remain
  non-authoritative. The focused test covers i32/i64 success and transactional
  malformed, missing, duplicate, cross-owner, kind/link/signature/count,
  zero-count, cast, and final-use failures.

## Proof

- Step 7.19 proof passed: fresh `cmake --build --preset default`; focused 2/2
  `ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'`. The supervisor
  owns canonical regression logs and any broader checkpoint.
