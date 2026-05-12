Status: Active
Source Idea Path: ideas/open/178_global_aggregate_layout_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select And Map One Global Layout Route

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting the bounded global aggregate
initializer layout route.

Selected route: metadata-rich `LirGlobal` aggregate initializer lowering for
top-level struct/union globals, from HIR global lowering into LIR globals and
then BIR global storage/initializer elements:

- Producer facts: `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
  `lower_global`, `lir_global_type_ref`, `lir_aggregate_structured_name_id`,
  and `build_type_decls` produce `LirGlobal::llvm_type_ref`,
  `LirModule::struct_decls`, `StructNameId`, `LirStructField::type`, and a
  legacy `type_decls` shadow.
- Consumer route: `src/backend/bir/lir_to_bir/module.cpp` `lower_module`
  builds `TypeDeclMap` plus `BackendStructuredLayoutTable`, then calls
  `src/backend/bir/lir_to_bir/globals.cpp` `lower_minimal_global_impl`.
- Layout/initializer consumers:
  `lookup_global_layout_result` / `lookup_global_layout` decide aggregate
  global size and alignment from `global.llvm_type`;
  `src/backend/bir/lir_to_bir/global_initializers.cpp`
  `lower_aggregate_initializer`,
  `lower_aggregate_initializer_recursive`,
  `append_zero_aggregate_initializer`, and
  `parse_global_gep_initializer` recursively interpret initializer payloads
  and pointer offsets through `lookup_global_initializer_layout`.
- Current rendered-spelling bridge: backend structured layout lookup is keyed
  by `std::string_view type_text`, normally `global.llvm_type` such as
  `%struct.Pair`. `build_backend_structured_layout_table` also bridges
  `StructNameId` through `struct_names.spelling(name_id)` into the same text
  key. When structured metadata is present, this rendered spelling still acts
  as the consumer-side semantic authority for choosing the layout entry; it
  must become display/compatibility lookup only, with `LirGlobal::llvm_type_ref`
  `StructNameId` or equivalent structured owner identity deciding the
  metadata-rich route.

## Suggested Next

Execute `plan.md` Step 2 for this selected route: thread the global aggregate
type identity from `LirGlobal::llvm_type_ref` into BIR global layout and
initializer lowering, so metadata-rich globals do not select structured layout
authority only by `global.llvm_type` rendered text.

## Watchouts

- Do not treat equal rendered `%struct...` spelling as sufficient identity for
  metadata-rich generated global aggregate paths.
- Keep legacy no-metadata compatibility explicit and isolated.
- Do not broaden into byval copy, AArch64 direct-LIR bridge retirement,
  function-pointer signature identity, or the closure gate.
- Do not weaken tests or mark supported global aggregate cases unsupported as
  proof of progress.
- Preserve the legacy fallback overloads of `lower_aggregate_initializer` and
  `parse_global_address_initializer` for raw/no-metadata inputs. The change
  should distinguish generated `LirGlobal::llvm_type_ref` metadata from
  compatibility `global.llvm_type` text instead of removing the text path.
- Nearby same-feature guards:
  `tests/backend/backend_prepare_structured_context_test.cpp`
  `check_aggregate_initializer_prefers_structured_layout_table` and
  `check_global_initializer_lowering_prefers_structured_layout_table`;
  `tests/frontend/frontend_lir_global_type_ref_test.cpp` global
  `StructNameId` and same-text/different-id equality checks;
  global pointer initializer handling through
  `parse_global_gep_initializer` and `resolve_pointer_initializer_offsets`;
  same-module aggregate global load consumers in
  `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`.
- Overfit guard: the later implementation must prove a stale or mismatched
  structured global identity case, not only a different `type_decls` text body
  with the same `%struct` spelling.

## Proof

Mapping-only packet. Per delegated proof contract, no build or CTest was run
and no `test_after.log` was created.

Targeted proof family for the later implementation packet:

- Build: `cmake --build build --target backend_prepare_structured_context_test frontend_lir_global_type_ref_test`
- Focused CTest: `ctest --test-dir build -R 'backend_prepare_structured_context|frontend_lir_global_type_ref' --output-on-failure`
- Add the selected mismatch coverage to the same focused family, with an
  optional x86 consumer check via `ctest --test-dir build -R 'backend_x86_handoff_boundary' --output-on-failure` if the implementation changes BIR global
  initializer shape consumed by same-module aggregate global loads.
