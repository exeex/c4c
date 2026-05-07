# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 2 is complete enough to advance. The parser handoff work now fences
rendered qualified `TextId` authority at helper boundaries, migrates qualified
template lookup through structured `QualifiedNameRef`/`QualifiedNameKey`
metadata, filters record-member typedef registration to unqualified owner
metadata, and keeps stale rendered spellings from populating compatibility
bridge keys. Reviewer report `review/147_step2_route_review.md` found the route
on track with no testcase-overfit.

## Suggested Next

Start Step 3 by inventorying the remaining qualified template and HIR
compatibility routes that can still reach rendered-spelling reconstruction.
Delegate one caller family at a time, replacing rendered qualified `TextId`
handoffs with structured carrier payloads and adding a focused stale-spelling
test when the migration changes behavior.

## Watchouts

- Do not treat helper renames, comment edits, or expectation rewrites as
  bridge-removal progress.
- Do not weaken qualified template or HIR tests to make the bridge deletion pass.
- `qualified_key_in_context()` still contains the rendered compatibility branch;
  Step 3 should burn down remaining semantic reachability before Step 4 deletes
  or isolates bridge helpers.
- Keep already-unqualified names authoritative. Use node-owned unqualified
  metadata only as a fallback when the delegated name is invalid/rendered, and
  never by suffix-splitting `A::B::C`.
- Remove no-structured-metadata fallback comments only after the corresponding
  structured route exists.
- Focus on qualified template carrier construction/lookup and HIR carrier or
  lookup paths identified by Step 1; display spelling is out of scope unless it
  is being used as semantic lookup authority.

## Proof

Suggested Step 3 proof family:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_|cpp_positive_sema_.*(template|alias|member_typedef|dependent_typename)'; } > test_after.log 2>&1`

The supervisor should capture a matching `test_before.log` before delegating
the next implementation packet so regression-guard comparison remains
monotonic.
