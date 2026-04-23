# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Narrow Prepared Debug And Admission Surfaces To Observational Adapters
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed step 3.3 by moving the prepared single-block return and minimal
direct-extern declaration surface from `x86_codegen.hpp` into
`lowering/return_lowering.hpp`, removing the duplicate prepared local-i16
guard declaration from the shared header, and rewiring `module_emit.cpp` to
consume the explicit prepared query header instead of relying on that
declaration leaking through `x86_codegen.hpp`.

## Suggested Next

Continue with the next bounded 3.3 packet by narrowing any remaining
prepared-only helper declarations that still sit in `x86_codegen.hpp` but are
not required by the inline bounded multi-defined holdouts, keeping the work to
declaration ownership and include rewiring only.

## Watchouts

- `x86_codegen.hpp` still needs the stack/local-slot operand declarations that
  its inline bounded multi-defined helpers call directly; do not try to strip
  those without first relocating or untangling the inline holdout surface.
- `module_emit.cpp` now depends explicitly on
  `prepared/prepared_query_context.hpp` for the prepared local-i16 guard
  declaration; avoid letting that dependency drift back through
  `x86_codegen.hpp`.

## Proof

Step 3.3 prepared return/declaration seam packet on 2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
