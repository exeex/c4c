Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate and Tighten Compatibility Boundaries

# Current Packet

## Just Finished

Step 5 is complete after the committed source-owner NTTP forwarding repair.
`resolve_ast_template_value_arg` now derives forwarded NTTP query keys from
`ctx.template_binding_owner_node` plus the forwarded `TextId` instead of from
the target template owner. Direct NTTP lookup bridges in scalar lowering,
value-arg expression evaluation, and generic control type inference now use
complete source-owner keys when the `FunctionCtx` owner and expression `TextId`
can identify the source NTTP parameter, while incomplete-metadata cases remain
on the documented compatibility path.

## Suggested Next

Begin `plan.md` Step 6 validation. Prove the full HIR template binding key
migration boundary, refresh the canonical proof log, inspect retained string
map compatibility comments, and look specifically for expectation weakening,
display-only changes claimed as semantic progress, named-case shortcuts, or
complete structured misses that still fall through to rendered/string maps.

## Watchouts

- `test_after.log` is not present in the current workspace even though the
  committed Step 5 packet records a green delegated run. Step 6 should refresh
  the canonical after log as part of validation.
- The focused `cpp_hir_template_parameter_binding_key_test` case proves a
  forwarded source-owner NTTP binding resolves even when the target owner key
  would be a structured miss.
- Direct NTTP sites converted in Step 5 require a complete source owner plus
  expression `TextId`; remaining no-key paths should be treated as
  compatibility bridges and checked against the source idea's reject signals.

## Proof

Step 5 recorded delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Recorded result: command exited 0 and `test_after.log` was written. The
delegated proof passed all 323 selected tests, including
`frontend_parser_lookup_authority_tests`, the `cpp_hir_.*` coverage, and the
positive template/qualified-template sema coverage.

Step 6 still needs fresh validation proof from the next executor packet.
