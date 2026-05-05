# Current Packet

Status: Complete
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Method Type-Pack Deduction

## Just Finished

Implemented the combined Step 5 HIR repair for member-template constructors.
Instantiated template structs now register all concrete-owner constructor
overloads before lowering constructor bodies, and constructor calls lower the
selected overload through a call-specific specialization path.

The specialization path consumes structured parser/HIR carriers directly:

- Concrete owner template bindings are stored on the constructor overload entry.
- Constructor overload lowering deduces method-template type packs and NTTP
  packs from structured callable parameter and argument `TypeSpec`
  / `TemplateArgRef` metadata.
- Signature substitution expands nested type-pack arguments inside aggregate
  parameter types such as `Owner<Pack...>`, so selected constructor bodies see
  concrete aggregate parameters instead of unresolved pack-owned structs.

This repairs the previous two-phase probe boundary: delegating constructor
lookup can now find the sibling constructor for the concrete owner and lower the
selected member-template constructor body with concrete type-pack/NTTP-pack
bindings.

## Suggested Next

Supervisor-side review/commit of the Step 5 slice, then advance the runbook to
the next Step 6 validation packet. No remaining Step 5 NTTP-pack blocker is
observable in the delegated positive-Sema proof.

## Watchouts

The two-phase registry is intentionally paired with selected constructor
specialization. Keeping only the registry half would regress back to the
observed `MemberExpr base has no struct tag (field='value')` boundary.

The repair uses structured template parameter indices/text ids and
`TemplateArgRef` value/type carriers. It does not recover constructor semantics
from rendered names, `_t` suffixes, `tag_ctx`, module dumps, or named testcase
patterns.

## Proof

Delegated positive-Sema proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: passed, 884/884. This improves the current `test_before.log` baseline
of 883/884 with no new failures in the delegated positive-Sema subset.

Proof log: `test_after.log`.
