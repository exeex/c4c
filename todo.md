Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Call Spelling And Printer-Side Bodies

# Current Packet

## Just Finished

Completed Step 4 call spelling ownership start. AST-backed checks showed
`print_call`, `print_call_boundary_move`, and
`print_call_boundary_abi_binding` depended only on generic printer result
construction, selected mnemonic lookup, register spelling, prepared value-home
spelling, and structured call/variadic records. Those bodies now live in
`calls.cpp` with declarations in `calls.hpp`; `machine_printer.cpp` keeps the
generic selected-node validation and variant routing switch, and routes call,
call-boundary move, and call-boundary ABI-binding payloads to the calls-owned
helpers. Direct `bl` and indirect `blr` text still comes from structured
`CallInstructionRecord` fields and the selected mnemonic, with no
assembly-text matching or semantic change.

## Suggested Next

Continue Step 4 by auditing whether any remaining printer-side helper-call or
call-result spelling is pure call ownership, while keeping runtime-helper-family
printers, generic register/memory spelling primitives, relocation helpers, and
variant dispatch outside calls unless a later packet explicitly owns them.

## Watchouts

- `machine_printer.cpp` still owns generic print-result wrappers, generic
  diagnostic header construction, selected-node validation, register/memory
  spelling primitives, relocation helpers, non-call family printers, runtime
  helper-family printers, and the top-level variant dispatch.
- `calls.cpp` now has a calls-local copy of the generic diagnostic/result
  helpers it needs for public call print functions; later movement should avoid
  turning generic printer helpers into calls-owned API unless a broader printer
  split is explicitly planned.
- I did not move i128/f128 runtime-helper printers in this packet. They emit
  call instructions internally, but their validation and surrounding emitted
  lines are runtime-helper-family behavior, not proven pure call spelling.
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
