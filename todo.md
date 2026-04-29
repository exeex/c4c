# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Convert Pure Parser Text Lookup To TextId

## Just Finished

Step 2 converted the record field duplicate-name checker from rendered
`std::string` set authority to parser `TextId` identity.

`parse_record_body()` now owns `std::unordered_set<TextId>` for
`field_names_seen`, and `make_record_field_duplicate_checker()` resolves field
spellings through the parser text table with fallback interning only when no
existing parser text id is present. Duplicate diagnostics still report the
original field spelling, and C++ mode still tolerates duplicate field names.

## Suggested Next

Next bounded conversion packet: pick the next pure parser-local string lookup
from the Step 1 inventory and convert only that local lookup to parser text
identity if it has a clean `TextId` boundary.

## Watchouts

Do not widen the next packet into `struct_tag_def_map`, record layout
const-eval helpers, template rendered mirrors, public support helper
signatures, or parser semantic record maps. The record field duplicate checker
kept its callback shape intentionally so this packet did not require parser
member-dispatch signature churn.

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
