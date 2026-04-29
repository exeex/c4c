# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split Remaining Parser Semantic Lookup From Text Spelling

## Just Finished

Step 2 is exhausted after the two pure parser-local text lookup conversions.

Completed Step 2 packets:

- K&R parameter declaration lookup now uses parser `TextId` identity instead
  of `std::string` map authority.
- Record field duplicate-name checking now uses `std::unordered_set<TextId>`
  for `field_names_seen`; diagnostics still report original field spelling,
  and C++ mode still tolerates duplicate field names.

Focused reinventory after those commits found no remaining obvious pure
parser-local text lookup map. Remaining string-keyed surfaces are
compatibility/final-spelling mirrors, public support-helper boundaries, or
semantic template/type binding state.

## Suggested Next

Next bounded Step 3 packet: convert the template specialization parameter
binding scratch state in `src/frontend/parser/impl/types/types_helpers.hpp`
from rendered parameter-name keys to parser/template parameter identity.

Scope for that packet:

- Target `select_template_struct_pattern()` and its local
  `type_bindings_map` / `value_bindings_map`, plus the helper lookup in
  `match_type_pattern()`.
- Prefer `TextId` from `Node::template_param_name_text_ids` when available;
  use the smallest fallback needed only where existing AST payloads lack a
  valid text id.
- Keep rendered names only as spelling/diagnostic or compatibility payloads
  for existing `std::vector<std::pair<std::string, ...>>` call boundaries if
  changing those callers would widen the packet.
- Add or run focused template specialization proof that stale rendered
  parameter spelling cannot override the selected parameter identity.

## Watchouts

Do not widen the next packet into `defined_struct_tags`, `struct_tag_def_map`,
record layout const-eval helpers, template rendered mirrors
(`template_struct_defs`, `template_struct_specializations`,
`instantiated_template_struct_keys`, `nttp_default_expr_tokens`), public
support helper signatures, or parser semantic record maps.

The template rendered maps already have structured-key companions and stale
rendered-name coverage; they are compatibility/Step 4 cleanup unless a Step 3
semantic packet proves a specific primary-authority leak.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contained the frontend parser subset output
and was rolled forward to `test_before.log` after supervisor review.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed before log roll-forward. No new failures.

Supervisor direct behavior check:

`ctest --test-dir build -j --output-on-failure -R '^(negative_tests_bad_(struct|union)_duplicate_field|cpp_positive_sema_eastl_slice6_template_defaults_and_refqual_cpp)$'`

Result: passed.
