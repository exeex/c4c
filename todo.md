Status: Active
Source Idea Path: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate And Prepare Completion Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 4 as evidence bookkeeping. The already-committed
focused tests prove generated call lowering uses structured metadata instead of
stale rendered call text when `LirCallOp::structured_args` is present.

The Step 3 backend coverage records four stale-text cases:
- direct scalar calls prefer structured argument type/operand facts over stale
  `args_str`;
- direct byval calls materialize aggregate ABI facts from
  `structured_args[index].type_ref` instead of stale rendered argument text;
- indirect calls prefer structured callee and argument metadata over stale
  rendered suffix or argument text;
- no-signature structured intrinsic calls infer argument facts from
  `structured_args` even when rendered signature metadata is absent and
  `args_str` is stale.

Raw/no-carrier compatibility remains explicit: empty `structured_args` keeps the
legacy rendered-text parser path, and the raw direct-call compatibility test
continues to prove hand-authored/no-metadata LIR still lowers through text.

## Suggested Next

Run `plan.md` Step 5: collect final validation/completion evidence for idea
190. Suggested proof is the already-green focused build/test command plus the
backend subset from Step 3, followed by any supervisor-requested broader
validation before closure.

## Watchouts

- Do not continue the idea 188 closure gate until ideas 190, 191, and 194 are
  closed or the closure-gate source idea is explicitly narrowed.
- `structured_args[].type_ref` is only populated when the existing
  `OwnedLirTypedCallArg` carried one. Some scalar and variadic-tail arguments
  currently have structured type/operand text but an empty `LirTypeRef`.
- Empty `callee_type_suffix` on a structured call is intentionally treated as
  absent parameter metadata, not as an authoritative zero-argument signature.
- The printer still uses `callee_type_suffix` and `args_str`, so stale rendered
  text can still affect printed LLVM text; Step 4 proves backend lowering
  authority, not printer spelling authority.
- Step 5 should decide whether the focused `frontend_lir_call_type_ref` plus
  `backend_lir_to_bir_notes` proof and the broader `^backend_` subset are
  enough to recommend closing idea 190, or whether the supervisor wants an
  additional regression guard before lifecycle closure.

## Proof

No new command was required for this evidence-only packet. Cited existing Step 3
proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green. Proof log: `test_after.log`.

Also cited existing Step 3 backend subset:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, `109/109` runnable tests green; `12` matching tests disabled.
