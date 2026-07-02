Status: Active
Source Idea Path: ideas/open/542_rv64_object_function_traversal_facade_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Facade Result And Admission Shell

# Current Packet

## Just Finished

Step 2 extracted the narrow prepared-function admission/result shell into
`prepared_function_emit.*`.

Moved/exposed only `prepare_rv64_object_function_admission_shell` plus its small
`RiscvPreparedFunctionAdmissionResult` and explicit diagnostic callback set. The
shell now owns prepared function name resolution, defined BIR body lookup,
empty-name/missing-body/atomic-operation rejection, prepared lookup construction,
dependency operand authority collection, select carrier-alias authority
collection, select-edge source producer placement collection, prepared
addressing/frame/storage/inline-asm lookup, stack-frame size admission,
saved-register/parameter-home/variadic admission callback sequencing, and
call-presence/call-frame size admissibility.

Retained object-side: `RiscvPreparedObjectFunctionResult`,
`RiscvObjectFunction`, prologue and formal-entry fragment appending, variadic
incoming-GPR fragment emission, before-return setup, traversal loops,
`fragment_for_prepared_instruction`, `fragment_for_prepared_terminator`, move
bundle/select-publication fragment bodies and diagnostics, public module/image
entrypoints, final object module assembly, data/symbol/fixup ownership, tests,
expectations, unsupported markers, and runtime contracts.

## Suggested Next

Execute Step 3 from `plan.md`: consider the next narrow context-assembly move
around prepared lookup/dependency setup only if dependencies stay explicit.
Keep traversal, fragment appending, and public object result ownership
object-side unless the supervisor delegates a narrower boundary.

## Watchouts

- Keep the next run behavior-preserving.
- Do not change admission semantics, diagnostics, block traversal order,
  function name matching, unsupported markers, runtime expectations, or object
  bytes.
- `prepared_function_emit.cpp` now has a narrow admission shell but must not
  become a second object-emission module.
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

Ran the delegated proof command and preserved output in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'
```

Result: passed, 63/63 tests.
