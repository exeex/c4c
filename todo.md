Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Domain-Identity Tests Or Probes

# Current Packet

## Just Finished

Step 5, "Add Domain-Identity Tests Or Probes", added focused HIR NTTP materialization probes for the structured owner/index/kind carrier.

- Added a same-owner NTTP carrier probe where `TemplateArgRef::debug_text` and rendered NTTP storage point at a stale value, proving owner/index/kind carrier lookup selects the parameter-domain binding instead.
- Added an equal-spelling foreign-owner NTTP probe where `Inner<N>` and `Outer<N>` share the same spelling/TextId/debug text; it fails if `TemplateArgRef::nttp_text_id`, `debug_text`, or current-primary rendered-name lookup can recover after the owner/index/kind carrier rejects the ref.
- The new probe exposed a real Step 5 blocker: typed NTTP materialization rejected the structured foreign-owner carrier, then fell back to string materialization. Tightened that fallback so structured NTTP carrier misses do not reopen rendered/debug/current-primary binding authority.

## Suggested Next

Next coherent packet: Step 6 should validate remaining compatibility boundaries and decide whether any string-keyed HIR binding adapters can be retired now or must stay as documented compatibility.

## Watchouts

- Remaining `find_template_typedef_binding` string-map reads are compatibility over legacy `TypeBindings` storage after owner/index matching, or legacy no-structured-carrier fallback; removal condition is migrating `TypeBindings` itself to domain-keyed storage.
- Remaining `nttp_bindings.find(param_name)` reads in HIR typed NTTP substitution are storage lookups only after owner/index/kind carrier validation; removal condition is migrating `NttpBindings` itself to structured parameter-domain storage.
- `Args1#0` / `Base#N` pack-series parsing remains compatibility-only for legacy explicit pack maps when no structured carrier exists or when a complete foreign owner/index carrier carries an outer pack binding through nested materialization; removal condition is replacing pack binding storage with structured parameter-domain pack entries.
- Remaining `debug_text` routes in touched HIR paths are display, deferred-expression syntax, numeric/bool literal parsing, no-carrier legacy type-argument compatibility, or the complete-foreign-carrier pack bridge above; Step 5 coverage now guards against using debug/text/current-primary lookup after a structured NTTP carrier miss.

## Proof

Ran delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_.*)$'`

Result: build succeeded; 111/111 tests passed. Proof log: `test_after.log`.
