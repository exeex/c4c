# Current Packet

Status: Active
Source Idea Path: ideas/open/148_hir_static_member_carrier_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Blocked HIR Static-Member Baseline

## Just Finished

Lifecycle switch completed from the blocked idea 147 Step 4 runbook to the new
HIR static-member carrier authority decomposition idea.

## Suggested Next

Next packet: execute Step 1 by inventorying the rejected Step 4 HIR failure
family, confirming the five known failing tests from `test_after.log`, and
recording the first focused HIR probe target for member identity versus owner
authority.

## Watchouts

- The failed proof log is preserved in `test_after.log`; do not overwrite it
  unless the supervisor delegates new proof output.
- The blocked idea 147 remains open. This plan exists only to define the HIR
  static-member carrier policy needed before returning to idea 147 Step 4.
- Known regressions to keep visible during Step 1:
  `cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`,
  `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`,
  `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`,
  `cpp_positive_sema_template_variable_trait_runtime_cpp`,
  and `cpp_hir_template_inherited_member_typedef_trait` failing.
- Rendered `expr->name` must not provide owner authority. Member identity may
  use `unqualified_text_id`, `unqualified_name`, or generated payload metadata
  only after structured owner authority exists.

## Proof

Lifecycle-only route reset. No code proof run for this plan-owner packet.
