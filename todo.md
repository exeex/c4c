# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 inventory classified the remaining qualified template and HIR
compatibility routes after Step 2. `qualified_key_in_context()` remains the
parser bridge boundary, but raw parser callers now mostly reject rendered
qualified `TextId`s before entering it. The first reachable semantic Step 3
work is in HIR template/callable paths, not parser helper deletion.

Classifications:

- Reachable semantic lookup authority:
  - `src/frontend/hir/impl/templates/templates.cpp::canonical_template_struct_primary()` still falls back through rendered `tpl_struct_origin`, derives a template family root by splitting `_T`, and strips `A::B::C` down to an unqualified family candidate.
  - `src/frontend/hir/hir_build.cpp::collect_initial_type_definitions()` registers template specializations by `item->template_origin_name` and, on miss, reconstructs a qualified origin string from rendered `item->name`.
  - `src/frontend/hir/hir_functions.cpp::substitute_signature_template_type()` has structured member-typedef owner-key lookup first, but still falls back to `member_typedef_compatibility_name()` plus `rfind("::")` and `resolve_struct_member_typedef_type(string, string)`.
  - `src/frontend/hir/hir_functions.cpp::prepare_callable_return_type()` and `append_explicit_callable_param()` fall back to `member_typedef_compatibility_name(owner_ts)` when owner metadata is incomplete.
  - `src/frontend/sema/consteval.cpp::lookup_record_layout()` is reachable from `compute_sizeof_type()` and `compute_alignof_type()`; it uses structured owner-index lookup first, then a rendered-tag canonicalization bridge, then a bare rendered-map fallback only when no owner index exists.
- Structured or already guarded:
  - `src/frontend/hir/hir_lowering_core.cpp::generic_type_compatible()` compares record owners via `make_record_owner_key_for_type()` before text-id fallback and does not split rendered qualified spelling.
  - `src/frontend/sema/type_utils.cpp::same_rendered_type_name_compatibility()` is guarded behind no-name-metadata checks inside `same_type_name_identity()` and is compatibility-only for legacy carriers.
  - `src/frontend/sema/validate.cpp::structured_record_key_for_type()` tries structured metadata first and only consults rendered-tag mirrors when `typespec_has_any_name_metadata(ts)` is false; the stricter `structured_record_key_for_type_metadata()` never consults rendered strings.
- Display-only or compatibility mirrors:
  - Parser `compatibility_spelling` assignments in `src/frontend/parser/impl/core.cpp` are display/fallback reporting once structured lookup has succeeded.
  - Sema rendered global/enum compatibility comparisons in `validate.cpp` compare legacy mirrors against structured results and are not first Step 3 authority.
- Step 4 deletion/isolation work:
  - `find_compatibility_key_from_rendered_qualified_spelling()`,
    `intern_compatibility_key_from_rendered_qualified_spelling()`, and the
    rendered branch inside `qualified_key_in_context()` should wait until the
    HIR/template semantic fallbacks above are migrated or isolated.

## Suggested Next

First implementation packet: migrate HIR template primary/specialization
authority away from rendered `tpl_struct_origin` strings.

Owned files should be limited to `src/frontend/hir/hir_build.cpp`,
`src/frontend/hir/impl/templates/templates.cpp`, focused HIR tests under
`tests/frontend/`, and `todo.md`. The packet should make
`collect_initial_type_definitions()` and `canonical_template_struct_primary()`
prefer `make_struct_def_node_owner_key()` / `template_origin_owner_key_hir()`
and owner-index maps, and stop deriving semantic family identity by splitting
rendered `item->name`, `tpl_struct_origin`, or `_T` suffixed spellings when
structured owner metadata is available. Add a stale-qualified-origin test that
would fail if `ns::Wrapper_T...` can resolve through suffix or scope splitting
instead of the owner key.

Suggested focused proof:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- `hir_build.cpp` and `hir_functions.cpp` are compiled according to
  `build/compile_commands.json`, so they are not stale draft-only files even
  though some comments describe staged split work.
- `hir_functions.cpp` contains multiple member-typedef compatibility fallbacks;
  keep them for a later packet after template primary/specialization owner-key
  routing is proven.
- `lookup_record_layout()` should not be the first packet: it is already
  owner-index-first and has a clear no-owner-index guard around bare rendered
  map fallback.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Inventory-only; no build required by packet.

Commands used for confidence:

`git status --short`

`sed -n '1,220p' plan.md`

`sed -n '1,220p' todo.md`

`rg -n "qualified_key_in_context|generic_type_compatible|same_rendered_type_name_compatibility|structured_record_key_for_type|lookup_record_layout|find_template_struct_primary|find_template_struct_specializations|find_template_global_primary|alias_template_key_in_context|member_typedef" src/frontend tests/frontend`

`rg -n "find_compatibility_key_from_rendered_qualified_spelling|intern_compatibility_key_from_rendered_qualified_spelling|split_qualified_name_scope|no structured|structured metadata|compatibility" src/frontend/hir src/frontend/sema src/frontend/parser/impl/core.cpp src/frontend/parser/impl/types/template.cpp src/frontend/hir/hir_build.cpp src/frontend/hir/hir_functions.cpp`

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/parser/impl/core.cpp qualified_key_in_context build/compile_commands.json`

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/hir/hir_lowering_core.cpp generic_type_compatible build/compile_commands.json`

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/sema/type_utils.cpp same_rendered_type_name_compatibility build/compile_commands.json`

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/sema/validate.cpp structured_record_key_for_type build/compile_commands.json`

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/sema/consteval.cpp lookup_record_layout build/compile_commands.json`

`rg -n "hir_build.cpp|hir_functions.cpp" CMakeLists.txt src tests build/compile_commands.json`

`c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/frontend/hir/hir_functions.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/frontend/hir/hir_build.cpp build/compile_commands.json`
