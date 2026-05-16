Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Call Spelling And Printer-Side Bodies

# Current Packet

## Just Finished

Completed Step 4 remaining printer-side helper-call and call-result spelling
audit. AST-backed caller/callee checks showed no additional clean pure
call-owned body to move: `print_i128_runtime_helper` and
`print_f128_runtime_helper` emit terminal `bl` lines, but their bodies also own
runtime-helper-family validation, live-preservation and selected-call policy
checks, marshal/unmarshal sequencing, f128 comparison result spelling, and
runtime-helper-specific support helpers. No code was moved in this packet; the
existing calls-owned `print_call`, `print_call_boundary_move`, and
`print_call_boundary_abi_binding` boundary remains intact.

## Suggested Next

Have the supervisor decide whether Step 4 is exhausted and should move to the
next plan step or receive a plan-owner/reviewer checkpoint before further
redistribution.

## Watchouts

- `machine_printer.cpp` still owns generic print-result wrappers, generic
  diagnostic header construction, selected-node validation, register/memory
  spelling primitives, relocation helpers, non-call family printers, runtime
  helper-family printers, and the top-level variant dispatch.
- Keep i128/f128 runtime-helper printers and their supporting helper move/result
  functions in `machine_printer.cpp` for now. They are runtime-helper-family
  behavior, not clean pure call spelling, even though they contain terminal
  helper `bl` emission.
- `calls.cpp` has the calls-local diagnostic/result helpers needed by its
  public call print functions; later movement should avoid turning generic
  printer helpers into calls-owned API unless a broader printer split is
  explicitly planned.
- Variadic `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`
  spelling moved with `print_call` because those forms are encoded as
  `CallInstructionRecord` payload variants and share the calls-owned call
  printer entry point.
- `machine_printer.hpp` did not require a change for this packet; the generic
  public machine-printer entry points stayed intact.

## Proof

Proof passed with log at `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The final run passed 139/139 `backend_` tests.
