Status: Active
Source Idea Path: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace The Owned Lowering Path

# Current Packet

## Just Finished

Step 2 from `plan.md`: traced the representative call-boundary move failure
from c-testsuite evidence into the AArch64 backend-owned call lowering path.

Representative trace, using `tests/c/external/c-testsuite/src/00125.c`:

- semantic input reaches prepared BIR as `bir.call i32 printf(ptr @.str0)`;
- prepared call plan records the direct extern variadic `printf` call with
  arg0 `source_encoding=symbol_address`, `source_value_id=1`,
  `source_symbol=@.str0`, `source_placement=gpr:caller_saved#0/w1`,
  `source_reg=x13`, `dest_placement=gpr:call_argument#0/w1`, and
  `dest_reg=x0`;
- prepared value locations also publish `@.str0 value_id=1 kind=register
  reg=x13`, so this is not missing BIR call-argument identity;
- `dispatch_prepared_block` finds the prepared call plan, emits
  `lower_before_call_moves(...)`, then emits the direct call;
- `lower_before_call_move(...)` in
  `src/backend/mir/aarch64/codegen/calls.cpp` constructs a
  `CallBoundaryMoveInstructionRecord`, but its selected GPR argument branch
  currently requires `argument->source_encoding == Register`;
- because the representative argument is `SymbolAddress`, that branch does not
  populate `source_register` or `destination_register`;
- `make_call_boundary_move_instruction(...)` therefore marks the node
  `DeferredUnsupported`, and `print_call_boundary_move(...)` fails with
  `call-boundary move node requires prepared register source and destination`.

Ownership classification: this is AArch64 backend lowering/selection work, not
frontend/BIR ownership. The prepared call plan carries enough source symbol,
source register, destination ABI register, and value-location facts to identify
the move. The missing support is the backend rule for symbol-address call
arguments at the call boundary: it must either lower/materialize the symbol
address into the destination call ABI register or produce a valid selected
register move only after authoritative address materialization is available.

Likely owned implementation files for the next packet:

- `src/backend/mir/aarch64/codegen/calls.cpp`: extend
  `lower_before_call_move` selection for symbol-address call arguments and keep
  `call_boundary_move_selection_status` honest for selected nodes.
- `src/backend/mir/aarch64/codegen/globals.cpp`: reuse or factor existing
  address materialization record/printing support if the rule materializes
  `@.str0` directly for the call argument.
- `src/backend/mir/aarch64/codegen/instruction.hpp` and focused
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` /
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` only if the
  implementation needs a distinct record shape or printer coverage rather than
  reusing existing address materialization and call-boundary move records.

## Suggested Next

Execute Step 3 by adding focused backend proof for a direct extern variadic call
whose first argument is a symbol/string address prepared as a GPR call argument.
The proof should fail on the old path because the before-call symbol-address
argument move reaches `DeferredUnsupported`, and should be independent of
c-testsuite filenames.

Keep the c-testsuite representative proof as the confirming subset:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00040|00056|00125|00131|00156)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- The call-boundary move family is currently reported as `[FRONTEND_FAIL]` by
  the c-testsuite runner because the assembly route reaches the machine-node
  printer and fails there. Treat it as the backend capability family owned by
  this runbook, not as runtime proof.
- The immediate missing support is not generic printer coverage for a valid
  selected node. The printer is correctly rejecting a call-boundary move record
  with no source/destination registers.
- Do not implement a filename-shaped shortcut. The semantic family is
  symbol-address call arguments, visible in the prepared call plan as
  `source_encoding=symbol_address`.
- Keep unrelated `[BACKEND_FAIL]` undefined temporary symbol cases and
  non-call-boundary frontend/BIR cases out of the next implementation packet.

## Proof

Delegated proof command:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00040|00056|00125|00131|00156)_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected the five representative
AArch64 backend c-testsuite cases and all five failed with the same
`[FRONTEND_FAIL]` machine-printer diagnostic:

`deferred_unsupported: call-boundary move node requires prepared register source and destination`

The shell pipeline exited 0 because the delegated command pipes through `tee`
without `pipefail`; the CTest result inside `test_after.log` is still 0/5
passed. Proof log: `test_after.log`.
