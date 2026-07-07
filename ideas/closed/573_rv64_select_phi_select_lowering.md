# RV64 Select And Phi-Select Lowering

Status: Closed
Type: Capability repair
Parent: `ideas/closed/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
Owning Layer: RV64 object lowering for BIR select materialization

## Closure Notes

Closed after the active runbook completed all five steps. RV64 object emission
now materializes scalar integer select values for the observed prepared-BIR
shape, including nested/phi-select publication, while reused no-home select
producer shapes remain fail-closed until a real publication strategy exists.

Focused object-emission tests cover simple scalar select behavior,
nested/phi-select publication, and unsupported/reused no-home fail-closed
cases. The `src/20030408-1.c` object-route representative passed after the
select materialization and select-edge cast rematerialization repairs, without
filename-, function-, block-, or value-name-specific handling.

Close-time backend regression guard passed with no new failures:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` reported
346/346 passing in `test_after.log`, compared against the matching green
backend baseline in `test_before.log`.

## Goal

Implement RV64 object-route lowering for BIR `SelectInst` values, with special
attention to prepared select chains that publish branch-join or phi-select
values.

## Why This Exists

The Step 3 diagnostics from the 570 runbook identified `src/20030408-1.c` as
a distinct select owner family:

- `function=test1`
- `block=logic.end.117`
- `instruction_kind=SelectInst`
- `owner=i32 %t126.phi.sel0`

Prepared BIR shows nested i32 select chains published from logic joins. This
is not evidence for generic branch lowering, call lowering, pointer arithmetic,
or runtime comparison work; the first object-route unsupported instruction is
the select value materialization itself.

Evidence:

- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20030408-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20030408-1.c/object-route.log`

## In Scope

- RV64 object emission for scalar integer `SelectInst` values.
- Materialization of nested select chains where the selected value is published
  as a prepared owner such as `%*.phi.sel*`.
- Focused tests for select result publication and branch-published phi-select
  shapes.
- Precise diagnostics for select operand or type forms that remain unsupported.

## Out Of Scope

- General branch lowering, CFG reconstruction, or BIR producer rewrites unless
  a focused blocker proves the select representation itself is invalid.
- Same-module calls, inline asm carriers, FP binary operations, pointer
  arithmetic, or runtime comparison work.
- Rewriting `src/20030408-1.c` expectations or adding a filename-specific
  shortcut.

## Acceptance Criteria

- Integer scalar select values in the observed prepared-BIR shape lower through
  RV64 object emission or fail with a narrower select-specific diagnostic.
- Tests cover both a simple scalar select and a nested/phi-select publication
  shape.
- Existing unsupported fallback behavior remains fail-closed for unhandled
  select types.
- The implementation does not depend on the representative filename or block
  name.

## Reviewer Reject Signals

- Reject a slice that handles only `%t126.phi.sel0` or the exact
  `logic.end.117` block shape.
- Reject claiming branch/CFG progress when the code only changes select
  diagnostics or expectations.
- Reject expectation downgrades, unsupported-marker edits, allowlist changes,
  or runtime-output changes as proof.
- Reject a BIR producer rewrite mixed into this idea unless the route first
  proves the existing prepared select is semantically invalid.
- Reject leaving the old generic `unsupported_instruction_fragment` failure as
  the first failure for scalar integer selects while claiming completion.
