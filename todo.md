# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory Constructor Specialization Seams

## Just Finished

Step 1 established the positive-Sema baseline for the member-template
constructor specialization decomposition plan.

The delegated baseline command ran and recorded the expected current state:
883/884 `^cpp_positive_sema_` tests pass, with the single expected failure:
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`.

## Suggested Next

Run Step 2 inventory/probe work: identify the parser, HIR, and constructor
lowering seams that must carry member-template constructor type-pack and
NTTP-pack specialization metadata, then record the first coherent repair
boundary before implementation.

## Watchouts

- Step 1 was baseline-only; no implementation files were touched.
- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not replace missing method-template or NTTP-pack bindings with
  named-case, rendered-name, `_t`, or `tag_ctx` recovery.
- Keep the final positive-Sema fixture as an end-to-end proof, not the only
  evidence of a semantic carrier repair.

## Proof

Delegated proof command:
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: expected red baseline. `test_after.log` records 883/884 passing with
only `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing.
