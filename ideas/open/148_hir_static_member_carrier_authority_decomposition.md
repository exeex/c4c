# HIR Static Member Carrier Authority Decomposition

Status: Open
Created: 2026-05-07

Related Ideas:
- `ideas/open/147_rendered_qualified_compatibility_bridge_removal.md`

## Goal

Define and prove the HIR static-member carrier policy needed before returning
to rendered qualified compatibility bridge removal.

Rendered `expr->name` spelling must not provide static-member owner authority.
Static-member member identity may still arrive through structured or
semi-structured carriers such as:

- `Node::unqualified_text_id`
- `Node::unqualified_name`
- `TypeSpec` qualifier segment metadata
- instantiated template static-member maps
- inherited static-member maps
- generated or member-typedef payloads that preserve value folding state

The goal is to split these authority seams into focused HIR probes and then
repair the carrier policy without reintroducing rendered owner/member splits.

## Why This Idea Exists

Idea 147 Step 4 is still valid, but its current runbook is too coarse for the
remaining HIR static-member route. Two executor attempts on the Step 4 HIR
static-member path moved the failure family instead of reducing it.

The post-HIR route review identified residual rendered owner/member splitting
in:

- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`

A local repair could remove the direct rendered `NK_VAR::name` split, but proof
regressed dependent and inherited trait/member-typedef static-member folding:

- `cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`
- `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`
- `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
- `cpp_positive_sema_template_variable_trait_runtime_cpp`
- `cpp_hir_template_inherited_member_typedef_trait`

This means the route needs smaller probes that distinguish owner authority from
member identity and distinguish primitive trait evaluation from instantiated or
inherited static-member payload lookup.

## In Scope

- Establish focused HIR probes for stale member `TextId` versus stale
  `unqualified_name` behavior.
- Separate primitive trait template-static evaluation from instantiated and
  inherited static-member maps.
- Preserve generated/member-typedef payload folding without allowing rendered
  owner splits.
- Define when member identity may use `unqualified_text_id`,
  `unqualified_name`, or generated payload metadata after structured owner
  authority is established.
- Return to idea 147 Step 4 only after the focused probes make the carrier
  policy explicit and provable.

## Out Of Scope

- Closing or completing idea 147.
- Deleting the original bridge-removal helpers before the HIR static-member
  carrier policy is proved.
- Broad parser or Sema carrier rewrites unrelated to the HIR static-member
  routes.
- Treating rendered `expr->name` owner/member splitting as an acceptable
  temporary authority path.
- Weakening dependent/inherited trait or member-typedef test contracts.

## Decomposition Seams

1. Member identity probes:
   distinguish stale `unqualified_text_id`, stale `unqualified_name`, and
   structured qualifier segment carriers in focused HIR tests.
2. Primitive trait evaluation:
   prove trait template-static folding does not need stale instantiated static
   maps before structured template-owner evaluation.
3. Instantiated and inherited static-member maps:
   prove generated/member-typedef payload folding can find the right member
   through structured owner carriers.
4. Bridge-removal return point:
   only resume idea 147 Step 4 after the above probes define which HIR carriers
   own static-member authority.

## Expected Probe Surfaces

- `tests/frontend/frontend_hir_lookup_tests.cpp` for focused HIR authority
  probes.
- Existing positive Sema/HIR cases for dependent trait and member-typedef
  folding as integration protection.
- No backend `tests/backend/case/` probe is expected for this idea unless the
  failure family later moves into backend lowering.

## Acceptance Criteria

- Focused tests prove stale rendered owner spelling cannot provide HIR
  static-member owner authority.
- Focused tests prove member identity behavior for `unqualified_text_id` and
  `unqualified_name` without using rendered owner splits.
- Primitive trait template-static evaluation is separated from instantiated
  and inherited static-member map fallback policy.
- Generated/member-typedef payload folding remains green for the known
  dependent and inherited cases.
- The route produces a clear handoff back to
  `ideas/open/147_rendered_qualified_compatibility_bridge_removal.md` Step 4.

## Reviewer Reject Signals

- A slice removes a visible rendered split but reintroduces owner authority via
  another rendered `A::B::member` spelling.
- A test passes because it uses a named-case shortcut for one trait or typedef
  fixture instead of proving a carrier policy.
- `Node::unqualified_name` or `Node::name` is treated as owner authority.
- Member identity fallback is accepted before structured owner authority has
  been established.
- Dependent or inherited trait/member-typedef regressions are handled by
  expectation rewrites, unsupported markers, or weaker test contracts.
- Broad parser, Sema, or backend rewrites are mixed into this HIR carrier
  decomposition without a focused HIR probe showing why they are required.
- The route claims progress by renaming helpers or comments while retaining
  the same rendered owner/member split behind the new name.
