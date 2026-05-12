Status: Active
Source Idea Path: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate And Prepare Completion Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 5 as final validation and completion evidence for
idea 190.

Implementation summary:
- `LirCallOp` now carries `structured_args` populated from generated
  `OwnedLirTypedCallArg` facts by `make_lir_call_op_with_return_type_ref`.
- LIR-to-BIR call lowering treats non-empty `structured_args` as the
  metadata-rich authority for argument operand, type text, and byval type-ref
  facts.
- Focused backend coverage proves stale rendered `args_str` or
  `callee_type_suffix` does not override structured metadata for direct scalar,
  direct byval, indirect, and no-signature structured calls.

Compatibility boundaries retained:
- Empty `structured_args` remains the explicit raw/no-metadata compatibility
  path through the legacy rendered-text parser.
- `callee_type_suffix` and `args_str` remain printed spelling and raw fixture
  payloads; this step does not claim printer spelling authority has moved to
  structured metadata.
- `structured_args[].type_ref` is carried only when the frontend-generated
  `OwnedLirTypedCallArg` already had a type ref; scalar and variadic-tail
  entries may still rely on structured type text plus operand identity.

Recommendation: idea 190 is ready for supervisor review and lifecycle close.
The source acceptance criteria are met: metadata-rich generated call lowering
can obtain argument facts without reparsing rendered call text as semantic
authority, stale rendered text cannot silently change selected argument or ABI
facts when structured metadata is present, raw/no-metadata compatibility remains
explicit, and focused direct plus indirect call coverage is green.

## Suggested Next

Supervisor should review this completed Step 5 packet, commit the coherent idea
190 slice if accepted, then delegate lifecycle close to the plan owner.

## Watchouts

- Do not continue the idea 188 closure gate until ideas 190, 191, and 194 are
  closed or the closure-gate source idea is explicitly narrowed.
- Idea 191 depends on idea 190 and can now proceed to byval signature-text
  retirement once supervisor/plan-owner close idea 190.
- Printer spelling still uses rendered call text; a future printer-authority
  initiative should be tracked separately if needed.

## Proof

Focused proof from the implementation slice:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green. Proof log: `test_after.log`.

Backend subset cited from Step 3:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, `109/109` runnable tests green; `12` matching tests disabled.

Final broad proof run for Step 5:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`

Result: passed, `3137/3137` runnable tests green; `12` disabled tests did not
run. Proof log: `test_after.log`.
