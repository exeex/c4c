# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 retried the `realize_template_struct()` stale rendered primary block.
The no-primary path now rejects a rendered primary candidate when the candidate's
complete owner key conflicts with complete structured origin-key metadata or a
template-primary record owner, limited to primary-shaped `TypeSpec`s without
pending template args so concrete realization compatibility stays intact.

Added focused HIR coverage in
`cpp_hir_template_canonical_primary_origin_metadata_test.cpp` for complete
origin-key miss rejection, record-owner miss rejection, and retained
no-metadata exact rendered realization compatibility.

## Suggested Next

Next Step 3 packet: inspect the remaining HIR compatibility callers that still
use rendered template primary or member-typedef fallbacks, and choose the next
narrow migration point that can be guarded by structured owner metadata without
touching Step 4 bridge removal.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- A broad direct `tpl_struct_origin` fallback block in `realize_template_struct()`
  still regresses local-declaration owner realization; this packet only blocks
  primary-shaped no-arg stale candidates whose rendered owner key conflicts with
  complete structured metadata.
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

Result: passed. `test_after.log` records 354/354 tests passing.
