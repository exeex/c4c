Status: Active
Source Idea Path: ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure inventory and handoff

# Current Packet

## Just Finished

Step 5 from `plan.md` completed: wrote the final closure inventory and
handoff notes for the rendered call-argument ABI suffix fallback cleanup.

Final parser inventory/classification:

- `src/backend/bir/lir_to_bir/calling.cpp`: `strip_call_arg_abi_type_suffix`
  remains as the narrow parser helper.
- `src/backend/bir/lir_to_bir/calling.cpp`:
  `legacy_raw_no_ref_call_arg_type_text` is the only production caller of the
  parser helper. The helper is fenced by
  `legacy_raw_no_ref_call_arg_fallback_allowed`, so structured call arguments
  with `type_ref`, `arg_type_refs`, or structured ABI payload cannot recover
  through rendered `alignstack(...)` text.
- `src/codegen/lir/call_args.hpp`: `format_lir_typed_call_arg` may still emit
  rendered `alignstack(...)` text for LIR formatting. This is a rendering
  surface, not semantic BIR call-lowering authority.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: remaining
  `alignstack(...)` occurrences are focused proof fixtures for the retained
  raw/no-ref compatibility path and for stale rendered suffix text losing to
  structured metadata.

Closure state: the parser was not removed, but it is retained only behind the
legacy/raw-no-ref fence. Structured supported call-argument lowering now takes
layout/alignment authority from structured metadata, with
`LirCallArg::aarch64_stack_align_bytes` carrying the AArch64 stack-alignment
fact for the covered carrier-array path.

## Suggested Next

Supervisor/plan-owner should not close the source idea yet unless they
explicitly accept the existing broader-validation blocker. Close-time review is
blocked by the pre-existing red 286/288 CLI subset until the separate semantic
BIR admission issue in the `match` load local-memory route is resolved or
split into its own lifecycle path.

## Watchouts

The rendered suffix parser cleanup itself is complete within this runbook's
owned boundary, but source-idea closure still depends on supervisor-owned
close-time validation policy.

The known 286/288 CLI subset remains red for a pre-existing issue outside this
packet: semantic BIR admission fails in the `match` load local-memory route.
Do not treat that blocker as evidence that structured call-argument lowering
still depends on rendered `alignstack(...)` suffix parsing.

No separate carrier follow-up was discovered for the rendered suffix parser.
The only residual boundary is the broader CLI semantic BIR admission blocker.

## Proof

Ran Step 5 inventory proof:

`rg -n "strip_call_arg_abi_type_suffix|legacy_raw_no_ref_call_arg_type_text|alignstack\\(" src/backend/bir/lir_to_bir/calling.cpp tests/backend/bir/backend_lir_to_bir_notes_test.cpp src/codegen/lir/call_args.hpp`

Result: passed. Full output is preserved in `test_after.log`.

Earlier focused proof from Step 4:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`

Result reported green by the prior executor packet.

The broader 286/288 CLI subset remains blocked by the pre-existing `match`
load local-memory semantic BIR admission failure and was intentionally not
rerun by this todo-only closure packet.
