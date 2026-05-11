Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Parser Compatibility Overloads

# Current Packet

## Just Finished

Step 1 - Reconfirm Bridge Inventory and First Target is complete. Re-read the
idea 167 final audit closure artifact and prior consolidated audit packet from
`git show 80c86e13d:todo.md`, then selected the first idea 168 implementation
target.

Selected first target bridge family:

| Field | Selection |
| --- | --- |
| Bridge owner | Parser support helpers in `src/frontend/parser/parser_support.hpp` and `src/frontend/parser/impl/support.cpp`. |
| Bridge family | Rendered typedef/type-compatibility overloads: `resolve_typedef_chain(TypeSpec, const std::unordered_map<std::string, TypeSpec>&)` and `types_compatible_p(TypeSpec, TypeSpec, const std::unordered_map<std::string, TypeSpec>&)`. |
| Current boundary | Parser-owned callers already have `TextId` overloads whose misses fail closed, but a rendered `std::string` overload remains as a broad compatibility API for legacy/HIR proof paths and production HIR callers that pass an empty rendered map. |
| Planned new boundary | Move production empty-map callers to the `TextId` overload with an empty `std::unordered_map<TextId, TypeSpec>`, then delete the rendered overloads if no non-test caller remains; if a caller still truly lacks metadata, rename/fence it as explicit no-metadata compatibility instead of leaving an ordinary overload. |
| Nearby tests | `cpp_hir_parser_type_helper_residual_structured_metadata`, `frontend_parser_lookup_authority_tests`, `frontend_parser_tests`, and `cpp_hir_template_pattern_match_structured_metadata`. |
| Implementation proof command | `cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests cpp_hir_parser_type_helper_residual_metadata_test cpp_hir_template_pattern_match_metadata_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_type_helper_residual_structured_metadata|cpp_hir_template_pattern_match_structured_metadata)$'` |

The target belongs to idea 168 because it narrows a retained rendered
source/type compatibility bridge from the idea 167 audit. It is not route-local
identity cleanup for idea 169 and does not require regression-guard machinery
from idea 170.

## Suggested Next

Implement the selected Step 2 parser-support packet: inspect production callers
of the rendered typedef/type-compatibility overloads, switch empty rendered-map
callers to the `TextId` overloads, and remove or explicitly fence any remaining
rendered overload surface.

## Watchouts

- Do not weaken the existing stale-rendered-map tests; they are regression
  witnesses for complete `TextId` misses.
- `src/frontend/hir/impl/templates/templates.cpp` and
  `src/frontend/hir/hir_functions.cpp` currently use empty rendered typedef
  maps only to satisfy overload selection, so they are likely first production
  caller updates.
- Keep consteval `by_name`, HIR local names, SSA values, labels, slots, and
  generated names out of this packet; those are idea 169 route-local work.
- If a real no-metadata rendered typedef caller appears during implementation,
  keep it explicit and named as compatibility rather than preserving the broad
  overload.

## Proof

Todo-only Step 1 packet. Delegated proof command:

`git diff --check -- todo.md`

Result: passed.

No implementation tests were run in this packet. `test_after.log` was not
updated because the supervisor delegated a todo-only diff-check proof command
with no log artifact.
