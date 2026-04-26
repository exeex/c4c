Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Safe Structured Layout Lookups First-Class

# Current Packet

## Just Finished

Completed `plan.md` Step 2 local direct dynamic member-array lookup guard.

This slice is coverage-only for an already structured-ready local aggregate
slot path: `local_direct_dynamic_member_array_load.c` now places `xs[3]`
behind a leading `char`, forcing dynamic member-array loads through a nonzero
field offset with padding. The semantic BIR guard now checks that the lowered
local slots are `%lv.p.4`, `%lv.p.8`, and `%lv.p.12` and still use dynamic
selects instead of falling back to LLVM `getelementptr`.

## Suggested Next

Continue `plan.md` Step 2 with the next safe structured layout lookup family,
preferably a real capability repair for pointer-parameter dynamic nested member
arrays or another small aggregate size/alignment or field-offset lookup that
can be proven without changing global lookup policy.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- An exploratory nested pointer-parameter case like `p->inner.xs[i]` currently
  lowered to only the first element at `addr %p.p+8`; that route was not kept
  in this slice because it needs a separate capability repair rather than a
  snippet update.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_codegen_route_x86_64_(nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|local_direct_dynamic_struct_array_call)_observe_semantic_bir$'; } > test_after.log 2>&1`

Result: passed. Build completed and 6/6 selected tests passed.
Proof log: `test_after.log`.
