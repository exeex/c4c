Status: Active
Source Idea Path: ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add focused proof coverage

# Current Packet

## Just Finished

Step 4 from `plan.md` completed: added focused proof coverage for the
structured metadata boundary and the retained legacy/raw-no-ref fallback fence.

Concrete changes:

- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: added raw/no-ref
  byval coverage proving rendered `alignstack(16)` suffix compatibility is
  retained only for legacy no-ref calls and does not become structured
  alignment authority.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: added a structured
  ABI-payload fail-closed case proving a `LirCallArg` with structured ABI
  metadata but no `type_ref` cannot recover through rendered byval text.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: tightened the
  AArch64 variadic carrier-array assertion so structured
  `aarch64_stack_align_bytes = 8` wins over stale rendered
  `alignstack(16)` text.

## Suggested Next

Execute Step 5 from `plan.md`: prepare closure inventory and handoff notes for
the supervisor/plan-owner path, including the remaining fenced parser use and
the known broader CLI blocker boundary.

## Watchouts

The named 286/288 CLI subset from the source idea remains blocked if rerun by
pre-existing semantic BIR admission failures in the `match` load local-memory
route; this packet intentionally did not fix that separate CLI dump blocker.

The remaining rendered suffix parser inventory is still two lines in
`calling.cpp`: the `strip_call_arg_abi_type_suffix` definition and a single
call inside `legacy_raw_no_ref_call_arg_type_text`. The retained use is covered
only as legacy/raw no-ref compatibility, with structured ABI-payload recovery
failing closed.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`

Result: build completed and `backend_lir_to_bir_notes` passed. Full output is
preserved in `test_after.log`.

The broader 286/288 CLI subset was not rerun in this executor packet because
the supervisor-provided context says it is already red before this slice on
pre-existing semantic BIR admission failures in the `match` load local-memory
route.
