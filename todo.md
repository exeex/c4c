Status: Active
Source Idea Path: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Route Generated LIR-To-BIR Calls Through Structured Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by routing LIR-to-BIR typed call parsing through
`LirCallOp::structured_args` when the carrier is present.

`BirFunctionLowerer::parse_typed_call` now builds argument views from
structured argument type text and operands before using rendered `args_str`.
Text parsing remains the empty-carrier compatibility path. Direct global call
parsing also uses this route when structured arguments exist.

Byval call argument layout lookup now prefers `structured_args[index].type_ref`
when present, while preserving legacy `arg_type_refs` and rendered text
fallbacks for raw compatibility. Focused backend tests make `args_str`
deliberately stale for direct scalar, direct byval, and indirect calls to prove
stale text cannot override structured argument facts.

Follow-up repair: no-signature calls with non-empty `structured_args` now infer
parameter facts from structured argument types when rendered suffix metadata is
absent, including builtin/intrinsic calls emitted with an empty
`callee_type_suffix`. Fixed rendered suffix entries are still validated when
present. Added focused coverage for a no-signature `llvm.fabs.double` call with
stale `args_str`.

## Suggested Next

Run the next supervisor-selected lifecycle packet for idea 190, likely a review
or Step 4 handoff if the runbook has remaining structured-call payload closure
work.

## Watchouts

- Do not continue the idea 188 closure gate until ideas 190, 191, and 194 are
  closed or the closure-gate source idea is explicitly narrowed.
- `structured_args[].type_ref` is only populated when the existing
  `OwnedLirTypedCallArg` carried one. Some scalar and variadic-tail arguments
  currently have structured type/operand text but an empty `LirTypeRef`.
- Empty `callee_type_suffix` on a structured call is intentionally treated as
  absent parameter metadata, not as an authoritative zero-argument signature.
- The printer still uses `callee_type_suffix` and `args_str`, so stale rendered
  text can still affect printed LLVM text until a later semantic proof separates
  backend authority from spelling.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green. Proof log: `test_after.log`.

Also ran:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, `109/109` runnable tests green; `12` matching tests disabled.
