Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve Prepared BIR And Existing Dump Contracts

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by proving the prepared-BIR and existing dump
contract subset selected by the supervisor after the structured render context
changes.

Concrete slice:
- Ran the build plus focused backend dump/prepared/prepare/route regex with
  explicit prefix wildcards for the CLI and prepare families.
- Confirmed existing CLI `--dump-bir`, prepared-BIR, backend prepare, and
  aggregate route contracts remain stable: 31/31 matching tests passed.
- No missing contract guard was exposed, so no tests or source files were
  changed.

## Suggested Next

Supervisor can move to `plan.md` Step 6 fallback inventory, or request broader
validation before that inventory if this slice is being treated as a milestone.

## Watchouts

- Prefix-style CTest families need explicit `.*` when combined with anchored
  alternatives; the corrected proof included the intended dump/prepared/prepare
  tests.
- This was a validation-only packet; no API seam or dump text change was
  needed.

## Proof

`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(backend_cli_dump_bir_.*|backend_cli_dump_prepared_bir_.*|backend_prepare_.*|backend_codegen_route_x86_64_(aggregate_param_return_pair|aggregate_return_pair|indirect_aggregate_param_return_pair|variadic_pair_second|local_aggregate_member_offsets)_observe_semantic_bir)$'; } > test_after.log 2>&1`
passed: build plus 31/31 selected tests. Proof log: `test_after.log`.
