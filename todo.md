# Current Packet

Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Route Public AArch64 Assembly Through The Common Printer

## Just Finished

Completed `plan.md` Step 5 by routing the public AArch64 assembly helper
through the shared hierarchical MIR printer. The route now passes each built
`MachineFunction` directly to `mir::print_machine_function(...)` with
`aarch64_codegen::MachineInstructionPrinter`, preserving the existing `.text`,
function symbol/type/size scaffolding, GNU-stack footer, and unsupported
diagnostic shape.

The public route no longer flattens `function.mir` into a temporary vector of
target instruction records just to print terminal assembly. Flat compatibility
views remain available for existing non-terminal compatibility callers.

## Suggested Next

Supervisor should decide the next packet from the active runbook. A coherent
follow-up would be review/closeout validation for idea 224, because the public
AArch64 assembly route now uses the common MIR printer while keeping the focused
CLI smoke output green.

## Watchouts

- The delegated owned-files list did not name `src/backend/backend.cpp`, but
  AST lookup showed it is the only public AArch64 assembly call site and both
  public AArch64 BIR/LIR entries call through it. The slice made the minimal
  required route-boundary edit there.
- The preserved failure text still says "machine-node printer" for compatibility
  with existing diagnostics, even though traversal now flows through the shared
  MIR printer.
- `FunctionRecord::machine_nodes` and other flat views remain compatibility
  projections; this packet did not alter those projections or broaden AArch64
  instruction coverage.
- Keep idea 229 as a follow-up markdown-shard conversion route unless the
  supervisor changes lifecycle direction.

## Proof

Delegated proof passed and wrote CTest output to `test_after.log`:

```bash
cmake --build build --target c4c_backend backend_aarch64_mir_carrier_test backend_aarch64_machine_printer_test c4cll && ctest --test-dir build -R '^(backend_aarch64_mir_carrier|backend_aarch64_machine_printer|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure > test_after.log
```

CTest subset: `backend_aarch64_machine_printer`,
`backend_aarch64_mir_carrier`, and
`backend_cli_aarch64_asm_external_return_add_smoke`, all passed.
