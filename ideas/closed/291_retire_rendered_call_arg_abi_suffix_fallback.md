# Retire Rendered Call-Argument ABI Suffix Fallback

## Goal

Finish the post-286 call-argument interface cleanup by retiring, quarantining,
or strictly fencing the remaining rendered `alignstack(...)` ABI suffix fallback
in semantic BIR call lowering.

The normal semantic path should consume structured call-argument type and ABI
metadata, not parse ABI facts out of rendered argument text.

## Why This Exists

Ideas 287 and 288 moved the selected AArch64 variadic HFA carrier path onto
structured metadata and a helper boundary, but 287's closure note left one
intentional compatibility tail:

- `alignstack` is structured for the selected AArch64 variadic HFA carrier
  path;
- rendered `alignstack(...)` text remains legacy compatibility for raw/no-ref
  rendered call arguments;
- `strip_call_arg_abi_type_suffix` is still used inside semantic BIR call
  lowering for aggregate layout selection.

That compatibility tail is now the next cleanup target. It should either
become an explicit legacy-only path with fail-closed proof, or disappear from
the supported semantic route once structured carriers cover the needed cases.

## In Scope

- Inventory every current caller of `strip_call_arg_abi_type_suffix` and
  classify it as:
  - structured call-argument path that should not parse rendered suffixes;
  - raw/no-ref legacy compatibility path;
  - test-only fixture construction gap;
  - separate carrier work.
- Decide whether `LirCallArg`, `LirTypeRef`, or a dedicated call-argument ABI
  metadata field should own `alignstack` for all supported aggregate call
  paths.
- Remove `strip_call_arg_abi_type_suffix` from structured semantic call
  lowering when structured metadata is present.
- If rendered suffix parsing must remain, quarantine it behind an explicit
  legacy/raw-no-ref predicate and add fail-closed tests proving structured
  mismatches cannot be rescued by rendered text.
- Preserve the completed 286/288 AArch64 variadic HFA carrier behavior.

## Out of Scope

- Rewriting AArch64 ABI classification or prepared call-plan construction.
- Reopening the AAPCS64 HFA lane helper boundary from idea 288.
- Migrating public prepared call-plan surfaces or deleting prepared fallback
  behavior.
- Treating hand-built test LIR convenience as evidence that production semantic
  lowering still needs rendered suffix parsing.
- Broad rendered-text cleanup outside call-argument ABI suffix ownership.

## Acceptance Criteria

- The closure note includes an inventory of all
  `strip_call_arg_abi_type_suffix` call sites and their final classification.
- Supported structured call-argument lowering no longer depends on rendered
  `alignstack(...)` text for semantic type/layout authority, or the remaining
  dependency is explicitly justified as legacy-only.
- Focused tests prove that structured call-argument metadata wins over stale or
  mismatched rendered suffix text.
- Focused tests prove any remaining raw/no-ref legacy fallback is quarantined
  and cannot broaden supported semantic lowering.
- The existing 286/288 proof subset stays green:
  `backend_lir_to_bir_notes`,
  `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

## Reviewer Reject Signals

- The patch merely renames `strip_call_arg_abi_type_suffix` or moves it to a
  different file while the same structured semantic path still parses rendered
  ABI suffixes.
- The patch accepts rendered `alignstack(...)` text as stronger authority than
  structured `LirCallArg`, `LirTypeRef`, or call-argument ABI metadata.
- The patch removes compatibility without proving raw/no-ref legacy cases fail
  closed or remain deliberately supported.
- The patch weakens 286/288 AArch64 variadic HFA coverage or rewrites expected
  output instead of fixing the interface boundary.
- The route expands into prepared call-plan retirement, AArch64 ABI policy
  rewrites, or unrelated rendered-text cleanup.

## Prior Lifecycle Hold

Parked on 2026-06-16 after the active runbook completed the suffix-parser
cleanup inventory, structured-metadata routing, legacy/raw-no-ref fence, and
focused boundary tests.

The source idea remained open at that checkpoint because its close-time 286/288
CLI proof subset was blocked by a separate reopened semantic BIR admission
failure in the `match` load local-memory route. That blocker was tracked
separately in `ideas/open/292_reopen_286_288_match_load_local_memory_admission.md`
before it was repaired and archived.

## Completion Notes

Closed on 2026-06-16 after the separate reopened 286/288 semantic BIR admission
blocker was repaired and archived under
`ideas/closed/292_reopen_286_288_match_load_local_memory_admission.md`.

The completed runbook retired the structured-route dependency on rendered
call-argument ABI suffix text, kept rendered `alignstack(...)` parsing fenced to
the raw/no-ref legacy fallback, and added focused fail-closed coverage proving
structured metadata wins over stale rendered suffix text.

Close-time proof confirmed the required 286/288 subset and adjacent focused
coverage are green:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$'`

Supervisor context also reported the broader `^backend_` guard improved from
175/5 to 177/3 with no new failures.
