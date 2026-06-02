# Current Packet

Status: Active
Source Idea Path: ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Equivalence And Prepare Closure Evidence

## Just Finished

Completed Step 4, "Prove Equivalence And Prepare Closure Evidence", as an
evidence-only packet for the aggregate-lane helper/table contraction. No
implementation file was changed in this packet.

Close-ready evidence summary:

- At the start of this packet, matching focused before/after logs were
  inspected at `test_before.log` and `test_after.log`; both contained passing
  results for `backend_aarch64_call_boundary_owner` and
  `backend_aarch64_machine_printer`.
- Focused proof command used for the code slices:
  `cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_machine_printer)$' --output-on-failure > test_after.log`.
- The focused tests cover the construction side for register-lane byval
  publication through `backend_aarch64_call_boundary_owner`, including
  register-sourced byval lane publication and frame-slot-sourced byval lane
  publication into ABI GPR lanes.
- The focused tests cover the printer side through
  `backend_aarch64_machine_printer`, including call-boundary move printing,
  frame-slot source loads, large frame-slot address materialization, and
  fail-closed printer diagnostics for missing prepared call-boundary facts.
- Step 4 first recorded closure evidence without changing implementation
  files. After that evidence-only note, the supervisor reran the focused proof
  into canonical `test_after.log`.
- `test_before.log` and the final supervisor-rerun `test_after.log` both
  contain passing `backend_aarch64_call_boundary_owner` and
  `backend_aarch64_machine_printer` results. Regression guard evidence passed
  2/2 focused tests before and 2/2 focused tests after.

Behavior equivalence notes:

- Register-sourced byval lane publication still derives the prepared source in
  `calls.cpp`, records `call_arg_byval_aggregate_register_lanes`, and leaves
  final lane assembly spelling to `machine_printer.cpp`.
- Frame-slot-sourced byval lane publication still requires a small selected
  byval lane extent, complete prepared selected source bytes when transport
  data exists, a printable prepared frame-slot source, and a GPR ABI
  destination before emitting the same aggregate-lane move record.
- Chunk-width load selection remains instruction-local through the
  8/4/2/1 candidate order in
  `aggregate_register_lane_printable_chunk(...)`, with descriptor publication
  selecting the load mnemonic and W/X load register from the chosen width.
- Destination-lane derivation still uses occupied destination register
  references first and then contiguous destination GPR indices, rejecting lanes
  that would exceed x30.
- Scratch exclusion still rejects reserved MIR scratch candidates that alias
  the destination register or any occupied destination register reference.
- Printer rejection paths are unchanged: aggregate-lane printing returns
  `std::nullopt` when the record is not an aggregate-lane publication, no
  scratch is available, no destination lane can be derived, no printable chunk
  descriptor exists, or the generic call-boundary printer facts are missing.

Helper/table surfaces intentionally left unchanged:

- `AggregateRegisterLanePrintableChunk` and
  `aggregate_register_lane_printable_chunk_descriptor(...)` remain public
  because `machine_printer.cpp` consumes the descriptor across a translation
  unit boundary while emitting aggregate register-lane publication.
- `aggregate_register_lane_scratch(...)`,
  `aggregate_register_lane_destination(...)`, and
  `is_aggregate_register_lane_publication(...)` remain public because the
  printer owns final aggregate register-lane emission, scratch use, OR
  assembly, byte-shifted lane placement, and fail-closed rejection.
- `aggregate_register_lane_memory(...)` and
  `aggregate_register_lane_memory_is_printable(...)` remain public because
  `calls.cpp` still uses them for construction-time validation of stack-lane
  inline-asm publication chunks.
- AArch64 byval lane classification, selected-lane extent, prepared-source
  validation, lane/chunk coverage, and size-limit authority remain in
  `calls.cpp`; those construction-owned decisions were not contracted.
- Stack-lane inline-asm publication and broad call-boundary record cleanup
  remain out of scope for this source idea.

Broader validation recommendation: before closure, supervisor-side validation
should run at least the same focused proof once after this evidence-only packet
or accept the current matching logs if no additional code changed. Broader
AArch64/backend validation is optional rather than required for this
evidence-only closure step because the active code changes were localized to
the AArch64 aggregate helper/public surface and the matching focused subset
already covers the named construction and printer paths.

## Suggested Next

Supervisor should review this close-ready evidence against
`ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md` and decide
whether to close the active plan, request reviewer scrutiny, or run broader
AArch64/backend validation before closure.

## Watchouts

- Preserve assembly output, diagnostics, unsupported-path contracts, scratch
  selection, and chunk-width selection.
- Treat this packet as closure evidence only; any additional implementation
  change should get fresh matching proof.
- Do not treat stack-lane inline-asm publication or broad call-boundary cleanup
  as complete under this idea.
- The current evidence supports behavior-preserving contraction, not a broader
  AArch64 backend milestone claim.

## Proof

Delegated Step 4 proof-text repair command passed and wrote
`/tmp/step4_todo_repair.log`:

`printf 'Step 4 todo proof text repair only; no implementation files changed.\n' > /tmp/step4_todo_repair.log && git diff --name-only >> /tmp/step4_todo_repair.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/(instruction|calls|machine_printer)\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during proof-text repair.\n' >> /tmp/step4_todo_repair.log; exit 1; fi`

This packet only repairs proof text in `todo.md`; it does not claim new
implementation changes. Step 4 first recorded closure evidence, then the
supervisor reran the focused proof into canonical `test_after.log`.
`test_before.log` and `test_after.log` both contain passing
`backend_aarch64_call_boundary_owner` and
`backend_aarch64_machine_printer` results, with regression guard evidence
passing 2/2 focused tests before and 2/2 focused tests after.
