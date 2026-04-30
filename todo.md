Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and Localize Structured Const Lookup Failure

# Current Packet

## Just Finished

Lifecycle repair reopened idea 136 after rejected closure baseline review and
activated a focused repair runbook for the structured static-member const
lookup regression.

## Suggested Next

Next coherent packet: execute Plan Step 1 by reproducing/localizing the
structured const-value lookup branch that bypasses instantiated trait/static
member evaluation and inherited base recursion.

## Watchouts

- Do not edit implementation files as part of lifecycle repair.
- Do not edit `review/*`, `test_before.log`, `test_baseline.log`, or
  `test_baseline.new.log`.
- Required focused regression scope includes:
  - `cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`
  - `cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`
  - `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
  - `cpp_positive_sema_template_variable_trait_runtime_cpp`
  - `cpp_hir_template_inherited_member_typedef_trait`
- The implementation target is structured static-member const lookup preserving
  instantiated trait/base recursion semantics while keeping structured keys as
  authority.

## Proof

Lifecycle-only repair. No build or test command was run by the plan owner.
