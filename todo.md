Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget Using Value Imports And Aliases

# Current Packet

## Just Finished

Step 3 - Retarget Using Value Imports And Aliases completed. Retargeted
`UsingValueAlias::target_key`, visible value alias lookup, direct namespace
value lookup, and `using` value import construction to prefer
`QualifiedNameKey` / structured value-binding lookup before falling back to
rendered compatibility names.

Focused parser tests now cover:
- using value imports still rendering the structured target when compatibility
  bridge spelling is corrupted
- using value aliases preferring the structured target type over a misleading
  compatibility bridge binding
- local value bindings shadowing using value aliases before structured alias
  resolution

## Suggested Next

Next coherent packet: Step 4 - Add Structured NTTP Default Cache Keys. Add a
structured cache identity for parser-owned NTTP default-expression tokens while
keeping the rendered string cache as a compatibility mirror.

## Watchouts

- Keep `find_var_type(const std::string&)`, `has_var_type(const std::string&)`,
  and `register_var_type_binding` behavior-compatible until downstream users are
  migrated deliberately.
- Preserve downstream rendered-name bridge behavior and compatibility spellings.
- The using-value paths now prefer structured value storage, but rendered
  compatibility names remain populated and used as fallback/output data.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
