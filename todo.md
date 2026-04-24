Status: Active
Source Idea Path: ideas/open/lir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Structural Validation and Cleanup

# Current Packet

## Just Finished

Step 5: Final Structural Validation and Cleanup completed the final
inspection and full validation pass for the LIR header hierarchy refactor:

- No obsolete top-level private headers remain for statement emission or
  constant initialization; `stmt_emitter.hpp` and `const_init_emitter.hpp` are
  absent from `src/codegen/lir/`.
- `src/codegen/lir/hir_to_lir/lowering.hpp` is the only private HIR-to-LIR
  header and acts as the shared private lowering index for the moved
  implementation files.
- No one-header-per-`.cpp` pattern was introduced under
  `src/codegen/lir/hir_to_lir/`; the moved implementation files share the
  private index instead.
- Top-level LIR headers are exported surfaces or public index headers:
  `ir.hpp`, `hir_to_lir.hpp`, `lir_printer.hpp`, `call_args.hpp`,
  `call_args_ops.hpp`, `operands.hpp`, `types.hpp`, and `verify.hpp`.
- `verify.hpp` is intentionally retained as a top-level LIR validation surface:
  it exports `verify_module` plus operand/type/render validation helpers used
  by `lir_printer.cpp`, not HIR-to-LIR private lowering state.
- Include inspection found no stale references to the retired top-level private
  headers and no public caller that needs `hir_to_lir/lowering.hpp`.

## Suggested Next

Supervisor should review and decide whether the active runbook is ready for
lifecycle closure or any final commit-boundary cleanup.

## Watchouts

`operands.hpp` and `types.hpp` were not part of the original private lowering
move, but they are top-level public LIR value/type surfaces used by other
top-level LIR APIs. No cleanup was needed for them in this packet.

## Proof

Full-suite proof command run as delegated:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure' > test_after.log 2>&1`

Result: passed. CTest reported 100% tests passed, 0 tests failed out of 3071
run tests, with 12 disabled tests not run. Proof log: `test_after.log`.
