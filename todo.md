Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Cover Aggregate-Sensitive Printer Paths

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by extending aggregate-sensitive BIR printer
coverage for a real LIR-to-BIR lowered aggregate call path.

Concrete slice:
- Added a `%struct.Pair` LIR fixture with structured declarations and a direct
  aggregate-return call that lowers through the normal sret call path.
- Verified the lowered BIR call carries `structured_return_type_name`, uses the
  expected memory-return ABI shape, and prints byte-stable `bir.call void ...`
  through the module structured context.
- Poisoned the lowered call's legacy `return_type_name` inside the test and
  proved the structured-backed printer path does not use it as active authority.
- Kept the existing handwritten BIR fallback guard intact.

## Suggested Next

Supervisor can decide whether Step 4 needs additional aggregate-sensitive
printer inventory, or whether this coverage slice is sufficient to move to the
next plan step.

## Watchouts

- This slice is coverage-only; no `src/**` changes were needed.
- The new lowered fixture proves the direct aggregate-return sret call path, not
  indirect calls or runtime intrinsic/inline asm fallback surfaces.
- Existing dump contracts still expect ABI `void` for the covered sret path and
  forbid source aggregate call spelling in CLI route tests.

## Proof

`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(backend_prepare_structured_context|backend_cli_dump_bir_(layout_sensitive_aggregate|is_semantic|00204_stdarg_semantic_handoff)|backend_cli_dump_prepared_bir_(is_prepared|00204_stdarg_prepared_handoff)|backend_codegen_route_x86_64_(aggregate_param_return_pair|aggregate_return_pair|indirect_aggregate_param_return_pair|variadic_pair_second)_observe_semantic_bir)$'; } > test_after.log 2>&1`
passed: build plus 10/10 selected tests. Proof log: `test_after.log`.
