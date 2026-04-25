Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Value Binding Storage

# Current Packet

## Just Finished

Step 2 - Add Structured Value Binding Storage completed. Added
`ParserBindingState::value_bindings` keyed by `QualifiedNameKey`, structured
`has_structured_var_type` / `find_structured_var_type` helpers, and structured
registration helpers. `register_var_type_binding` now dual-writes structured
value storage for non-local bindings while preserving the existing rendered
string tables. Structured registration also backfills the rendered-string legacy
path where the key can render. Fixed the Step 2 acceptance blocker by including
`value_bindings` in the heavy tentative `ParserSnapshot` save/restore path.

Focused parser tests now cover:
- existing string-facing value lookup still recovering the stored `TypeSpec`
- string registration also populating structured value storage
- qualified value registration agreeing between structured and legacy lookup
- direct structured value registration keeping legacy string lookup populated
- heavy snapshot rollback removing structured value bindings added after a
  snapshot, including the direct structured registration path

## Suggested Next

Next coherent packet: Step 3 - Retarget Using Value Imports And Aliases. Prefer
structured value binding lookup through `UsingValueAlias::target_key` and using
import resolution while keeping rendered compatibility names as bridge/output
data.

## Watchouts

- This packet intentionally did not retarget `using` import behavior or general
  visible value lookup to the structured map; snapshot rollback is now ready for
  production reads to start depending on `value_bindings` in a later packet.
- Keep `find_var_type(const std::string&)`, `has_var_type(const std::string&)`,
  and `register_var_type_binding` behavior-compatible until downstream users are
  migrated deliberately.
- Preserve downstream rendered-name bridge behavior and compatibility spellings.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
