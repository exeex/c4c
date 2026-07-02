Status: Active
Source Idea Path: ideas/open/542_rv64_object_function_traversal_facade_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Function Traversal Facade Ownership

# Current Packet

## Just Finished

Step 1 mapped the smallest behavior-preserving facade boundary around
`prepared_function_to_object_function` without implementation edits.

First safe extraction target: a narrow prepared-function admission/result shell
in `prepared_function_emit.*`, not the traversal loop. The shell should cover
only the early prepared-function admission and facade input gathering that can
keep dependencies visible: prepared function name resolution, defined BIR body
lookup, empty-name/missing-body/atomic-operation rejection, variadic admission
diagnostic preservation, prepared lookups, dependency operand authorities,
select carrier alias authorities, select-edge source producer placements,
addressing/frame/storage/inline-asm lookup, stack-frame sizing, saved-register
diagnostics, parameter-home diagnostics, variadic-helper diagnostics, and
call-presence/call-frame admissibility. Keep `RiscvPreparedObjectFunctionResult`
and `RiscvObjectFunction` ownership object-side unless the next slice introduces
a small facade-local result that does not pull final object assembly across the
boundary.

## Suggested Next

Execute Step 2 from `plan.md`: extract only the admission/result shell into
`prepared_function_emit.*` with explicit parameters/results, keep all fragment
appending and traversal object-side, then run the exact validation command
recorded below.

## Watchouts

- Keep this run behavior-preserving.
- Do not change admission semantics, diagnostics, block traversal order, function name matching, prepared lookup construction, unsupported markers, runtime expectations, or object bytes.
- `prepared_function_emit.cpp` is already compiled by `src/backend/CMakeLists.txt`;
  use the existing file for Step 2 unless a later slice has a concrete reason to
  add a new compiled facade file.
- Prior family APIs are available for the boundary: stack frame sizing and
  prologue helpers from `prepared_frame_emit.*`, before-return move classification
  and variadic resource predicates from `prepared_call_emit.*`, and select
  predecessor/carrier-alias predicates from `prepared_edge_publication_emit.*`.
- The Step 2 shell can make these dependencies explicit by passing the prepared
  module, control-flow function, discovered BIR function/name, prepared lookups,
  authority records, addressing/frame/storage/inline-asm pointers, stack-frame
  bytes, and call-presence facts. Avoid a carrier struct that hides those fields
  unless it is a small named value object with no object encoder authority.
- Keep `fragment_for_prepared_instruction`, `fragment_for_prepared_terminator`,
  `fragment_for_prepared_move_bundle`, predecessor select-publication fragment
  bodies, formal-entry-home and variadic incoming-GPR fragment bodies, block label
  fragment emission, final object module assembly, public ELF entrypoints, data
  object emission, symbol/fixup ownership, relocation mapping, section emission,
  and module layout parked unless a later plan-owner/reviewer decision explicitly
  changes scope.
- Diagnostic-heavy object context remains parked for now: select-publication
  bundle rejection formatting, move-bundle fragment failure formatting,
  instruction-fragment fallback diagnostics, and object consumer diagnostic
  category propagation from traversal events.
- Missing/parked dependency: `fragment_for_prepared_instruction` still fans out
  through object encoder utilities and multiple fragment families, so Step 2 must
  not hide it behind a new facade dispatcher.
- Do not hide dependency sets behind a new catch-all facade or second all-purpose object emission module.

## Proof

No build or test proof was required for this mapping-only packet; no
`test_after.log` was written.

Exact Step 2 validation command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'
```
