# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 migrated the signature template substitution path for nested template
owner structs. `substitute_signature_template_type()` now refreshes structured
`tpl_struct_origin_key` owners into canonical primary names during recursive
signature substitution, including nested `Wrap<Wrap<T>>` and `Box<Pair<T>>`
shapes with stale rendered spellings.

`resolve_signature_template_type_if_needed()` now realizes concrete nested
template signature types even when no template bindings are present, so
non-template return signatures such as `Wrap<Wrap<int>>` are not left as
`struct<?>`. `realize_template_struct()` also updates realized nested concrete
args back into `ResolvedTemplateArgs::type_bindings`, preventing `Box<Pair<int>>`
from being reused for the `T=long` `Box<Pair<T>>` specialization.

Added focused HIR coverage in `frontend_hir_lookup_tests.cpp` for stale rendered
nested owner spellings losing to structured owner/name metadata during
signature substitution.

## Suggested Next

Next Step 3 packet: with nested signature owner substitution proven, retry the
`realize_template_struct()` stale rendered primary block for complete structured
origin-key and record-owner metadata while keeping legacy no-metadata exact
rendered compatibility intact.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- A broad direct `tpl_struct_origin` fallback block in `realize_template_struct()`
  still regresses local-declaration owner realization; the next packet should
  keep the guard narrowly tied to complete structured misses.
- `hir_functions.cpp` contains multiple member-typedef compatibility fallbacks;
  keep them for a later packet after template primary/specialization owner-key
  routing is proven.
- This slice intentionally leaves exact rendered template primary lookup,
  string-key template primary maps, and parity probes in place; Step 4 owns
  deletion/isolation after remaining HIR callers are migrated.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 354/354 tests passing, including
`cpp_positive_sema_template_angle_bracket_validation_cpp` and
`cpp_positive_sema_template_struct_nested_cpp`.
