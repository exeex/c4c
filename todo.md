Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Preserve Downstream Bridge Behavior

# Current Packet

## Just Finished

Step 7 - Preserve Downstream Bridge Behavior completed as an audit-only slice.
No code or test changes were needed.

Confirmed downstream-bound rendered identities from Steps 2-6 remain preserved:
`Node::name` stays the rendered AST/HIR-facing declaration and instantiated
struct name, while structured additions live beside it in `unqualified_text_id`,
qualifier TextIds, namespace context, and parser lookup keys. `TypeSpec::tag`
continues to carry rendered typedef, struct, enum, and instantiated-template
tags, including synthesized typedef tags and template instantiation results.
Typedef and value bridge names remain populated through compatibility string
tables while structured/TextId lookups are preferred. `UsingValueAlias` keeps
`target_key` as semantic identity and `compatibility_name` as rendered bridge
data, with visible-name output rendered from the structured key when possible
and falling back to the bridge only when needed. Template instantiation de-dup
now mirrors structured keys and rendered keys without changing final instantiated
names or returned tags. Known-function registration and lookup use structured
keys for parser identity while preserving the rendered spelling exposed through
`has_known_fn_name(string)` and visible value resolution.

Existing focused parser coverage already proves the bridge behaviors with
mismatched/corrupted fallback spellings for typedef lookup, value lookup, using
value aliases/imports, using type imports, known-function names, NTTP cache
mirrors, and template instantiation tag reuse.

## Suggested Next

Next coherent packet: supervisor should route Step 8 only if the runbook is
ready to demote parser-owned legacy string paths after reviewing the Step 2-7
matching proof.

## Watchouts

- Follow-on cleanup belongs in later ideas or Step 8 demotion, not this Step 7
  bridge audit: remove or narrow compatibility string mirrors only after stable
  matching proof, and leave broad AST/HIR/sema/codegen rendered-name model
  cleanup to a separate downstream initiative.
- Public string overloads and rendered output fields are still intentional
  bridge surfaces for downstream consumers and diagnostics.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
