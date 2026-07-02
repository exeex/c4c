Status: Active
Source Idea Path: ideas/open/542_rv64_object_function_traversal_facade_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Object Function Traversal Loop Facade

# Current Packet

## Just Finished

Step 4 reviewed the object-function traversal loop boundary and made no code
changes because no small loop facade could reduce central coupling without
hiding object-emission dependencies.

The complete traversal path still directly owns event ordering from
`prepare::make_prepared_object_function_traversal`, block label fragment
emission, prepared-consumer diagnostics, move-bundle classification,
select-publication admission/fragment selection, before-return
stack-to-register validation state, instruction fragment emission and fallback
diagnostics, terminator fragment emission, and fragment append order.

The fallback block traversal path still directly owns label emission order,
instruction fragment fallback diagnostics, terminator fragment emission, and
fragment append order for the non-complete traversal stream.

Moving the loop into `prepared_function_emit.*` now would require either
moving `RiscvPreparedObjectFunctionResult`, `RiscvObjectFunction`, and the
object-local fragment helpers with it, or passing a broad callback/context
bundle for `fragment_for_prepared_instruction`,
`fragment_for_prepared_terminator`, move-bundle/select-publication fragment
bodies, diagnostics, compare state, before-return state, and append targets.
That would recreate the central coupling behind a new facade instead of making
the dependencies clearer.

## Suggested Next

Execute Step 5 from `plan.md`: review close readiness for the active source
idea. Confirm the Step 2 admission shell is the only safe extraction from this
runbook, Steps 3 and 4 were intentionally no-code, and remaining traversal loop,
fragment fanout, diagnostics, public entrypoints, final module assembly, and
symbol/fixup/module ownership are parked without weakening behavior.

## Watchouts

- Keep the next run behavior-preserving and review-only unless the supervisor
  delegates a specific follow-up.
- Do not change admission semantics, diagnostics, traversal order, function name
  matching, unsupported markers, runtime expectations, or object bytes.
- `prepared_function_emit.cpp` now has a narrow admission shell but must not
  become a second object-emission module.
- Step 4 intentionally made no code change: moving traversal loops now would
  either move object-owned fragment/result helpers out of scope or hide them
  behind a broad callback/context bundle.
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
  through object encoder utilities and multiple fragment families, so this
  runbook must not hide it behind a new facade dispatcher.
- Do not hide dependency sets behind a new catch-all facade or second all-purpose object emission module.

## Proof

No proof command was run and no new `test_after.log` was written because Step 4
made no code changes, per the delegated packet.

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'
```

Result: not run; no-code rationale recorded above.
