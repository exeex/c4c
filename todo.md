Status: Active
Source Idea Path: ideas/open/542_rv64_object_function_traversal_facade_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Traversal Context Assembly

# Current Packet

## Just Finished

Step 3 reviewed the next traversal context assembly boundary and made no code
changes because Step 2 already moved the safe dependency-gathering/context
assembly into `prepared_function_emit.*`.

Confirmed with AST-backed callee inspection that
`prepare_rv64_object_function_admission_shell` already owns prepared function
name resolution, defined BIR body lookup, prepared lookup construction,
dependency operand authority collection, select carrier-alias authority
collection, select-edge source producer placement collection, prepared
addressing/frame/storage/inline-asm lookup, stack-frame sizing admission, and
call-presence/call-frame admissibility, with those dependencies exposed as
concrete `RiscvPreparedFunctionAdmissionResult` fields.

Retained object-side: `RiscvPreparedObjectFunctionResult`,
`RiscvObjectFunction`, prologue and formal-entry fragment appending, variadic
incoming-GPR fragment emission, before-return stack-to-register validation
state, `prepare::make_prepared_object_function_traversal` invocation and
complete/fallback traversal loops, block label fragment emission,
`fragment_for_prepared_instruction`, `fragment_for_prepared_terminator`, move
bundle/select-publication fragment bodies and diagnostics, public module/image
entrypoints, final object module assembly, data/symbol/fixup ownership, tests,
expectations, unsupported markers, and runtime contracts.

No narrower move remained for Step 3 without either duplicating the Step 2
admission result or introducing a catch-all traversal/facade carrier that would
hide fragment and object-emission dependencies.

## Suggested Next

Execute Step 4 from `plan.md`: evaluate the smallest object-function traversal
loop facade only if event order, diagnostics, and fragment dependencies remain
explicit. Keep `fragment_for_prepared_instruction`, final object module assembly,
public entrypoints, and symbol/fixup/module ownership parked unless the
supervisor delegates a reviewed boundary.

## Watchouts

- Keep the next run behavior-preserving.
- Do not change admission semantics, diagnostics, block traversal order,
  function name matching, unsupported markers, runtime expectations, or object
  bytes.
- `prepared_function_emit.cpp` now has a narrow admission shell but must not
  become a second object-emission module.
- Step 3 intentionally made no code change: moving more context assembly now
  would either repeat the explicit admission shell fields or hide traversal and
  fragment dependencies behind a broad carrier.
- Prior family APIs are available for the boundary: stack frame sizing and
  prologue helpers from `prepared_frame_emit.*`, before-return move classification
  and variadic resource predicates from `prepared_call_emit.*`, and select
  predecessor/carrier-alias predicates from `prepared_edge_publication_emit.*`.
- The Step 2 shell exposes the prepared module/control-flow facts as a small
  result with concrete fields, not an encoder-authority carrier.
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

No proof command was run and no new `test_after.log` was written because Step 3
made no code changes, per the delegated packet.

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'
```

Result: not run; no-code rationale recorded above.
