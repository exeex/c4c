# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Positive-Sema Baseline

## Just Finished

Lifecycle switch created and activated the member-template constructor
specialization metadata decomposition idea. The previous TypeSpec-tag deletion
plan is parked open, not closed.

## Suggested Next

Run Step 1 baseline:
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Confirm the expected 883/884 positive-Sema state with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing, then record the result here before implementation.

## Watchouts

- Do not touch implementation files during lifecycle-only activation.
- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not replace missing method-template or NTTP-pack bindings with
  named-case, rendered-name, `_t`, or `tag_ctx` recovery.
- Keep the final positive-Sema fixture as an end-to-end proof, not the only
  evidence of a semantic carrier repair.

## Proof

Lifecycle-only switch. No build or test command was run by the plan owner.
