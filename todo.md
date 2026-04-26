Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add BIR Dump Guards For Layout-Sensitive Paths

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by adding
`backend_cli_dump_bir_layout_sensitive_aggregate`, a focused `--dump-bir`
guard using the existing `aggregate_param_return_pair.c` backend fixture.

The new dump guard checks stable layout-sensitive aggregate facts already
lowered into BIR: callee `sret(size=8, align=4)` and
`byval(size=8, align=4)` ABI annotations, the caller aggregate ABI call, and
`+4` aggregate copy offsets for the byval input and sret output.

## Suggested Next

Continue Step 5 with one more focused `--dump-bir` guard only if the supervisor
wants broader dump coverage, preferably for aggregate `va_arg` copy offsets;
otherwise move to the next planned validation or lifecycle packet.

## Watchouts

- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- Keep dump snippets tied to existing semantic BIR facts such as sret/byval
  size/align annotations and aggregate copy offsets.
- Do not force HFA-like coverage into semantic BIR text unless the route starts
  exposing stable target HFA classification facts.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(backend_cli_dump_bir_(layout_sensitive_aggregate|is_semantic|vla_goto_stackrestore_cfg|00204_stdarg_semantic_handoff)|backend_codegen_route_x86_64_(aggregate_param_return_pair|variadic_pair_second)_observe_semantic_bir)$'; } > test_after.log 2>&1`

Result: passed. Build completed and 6/6 selected tests passed.
Proof log: `test_after.log`.
