Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Structured-First Layout Access Helpers

# Current Packet

## Just Finished

Step 2 first target completed: demoted
`ConstInitEmitter::lookup_const_init_struct_def` so
`lookup_structured_layout(..., "const-init-aggregate")` is retained only for
structured-layout observation/parity reporting. The const initializer no
longer selects the aggregate declaration through `layout.legacy_decl`; it now
continues directly to the existing `mod_.struct_defs` lookup and fallback
behavior.

## Suggested Next

Next coherent packet should address one remaining active semantic
`legacy_decl` authority site after the supervisor selects the route. The
size/alignment users still need a structured metadata or parity-gated helper
decision before demotion.

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage; do not remove
  `StructuredLayoutLookup::legacy_decl`.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- Const-init demotion is intentionally behavior-preserving because the removed
  `layout.legacy_decl` branch and the retained fallback both resolve through
  `mod_.struct_defs` for valid aggregate tags.
- Size/alignment demotions remain deferred: `LirStructDecl` currently carries
  field type refs but not explicit size/align metadata, so `object_align_bytes`,
  variadic aggregate payload sizing, and `va_arg` aggregate payload sizing need
  a parity-gated helper/data decision before their `legacy_decl` reads can be
  safely demoted.

## Proof

Passed delegated proof, with output preserved in `test_after.log`:
`(cmake --build build --target c4cll && ctest --test-dir build -R 'frontend_lir_global_type_ref|cpp_hir_record_field_array_layout|cpp_hir_record_packed_aligned_layout|cpp_positive_sema_inherited_base_aggregate_init_runtime_cpp' --output-on-failure) > test_after.log 2>&1`
