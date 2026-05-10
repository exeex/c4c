Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Validation Checkpoint

# Current Packet

## Just Finished

Step 8 completed the validation checkpoint for the canonical symbol identity
route.

The selected sema metadata subset passed after a fresh build:
`cpp_hir_sema_canonical_symbol_structured_metadata`,
`cpp_hir_sema_consteval_type_utils_structured_metadata`, and
`cpp_positive_sema_lookup_value_structured_metadata`.

The narrow canonical proof was already green through Step 7, covering
same-spelled template parameters with different owners, nominal and record
identity separation across structured domains, canonical identity/hash and
symbol-table lookup separation, and ABI/debug rendering paths staying
output-only.

## Suggested Next

Supervisor should decide whether the active runbook is exhausted and route the
next lifecycle action.

## Watchouts

- `test_baseline.new.log` remains rejected as a full-suite acceptance baseline
  because it contains the known non-canonical-symbol failures
  `frontend_parser_lookup_authority_tests`,
  `cpp_c4_static_assert_if_constexpr_branch_unlocks_later`, and
  `cpp_c4_static_assert_multistage_shape_chain`.
- Retained rendered-string fallback: diagnostic, debug, ABI, and compatibility
  display strings remain output renderers only. Removal condition: delete any
  semantic dependency on these strings once the downstream consumer has a
  structured identity or explicit output-only API.
- Retained rendered-string fallback: no-metadata compatibility still uses
  rendered names only when owner-aware structured metadata is absent. Removal
  condition: require complete structured metadata for that domain and reject or
  normalize missing metadata before lookup/equality/hash decisions.
- Retained rendered-string fallback: compatibility formatting may preserve
  stable rendered spelling for external text output. Removal condition: replace
  callers that use formatted text as a lookup key with `CanonicalIdentity`,
  `types_equal`, or domain-specific structured metadata.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed. Full combined output is preserved in `test_after.log`.
