Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Layout And Build References

# Current Packet

## Just Finished

Activated `ideas/open/92_parser_agent_index_header_hierarchy.md` into
`plan.md` and initialized this execution state for Step 1.

## Suggested Next

Start Step 1 by inventorying the current parser file tree, build references,
and include references for the top-level parser implementation files that will
move under `src/frontend/parser/impl/`.

## Watchouts

- This is structural parser layout work; preserve parser behavior, AST output,
  diagnostics behavior, visible-name behavior, pragma behavior, template
  parsing behavior, and testcase expectations.
- Do not split `ast.hpp`.
- Do not create one-header-per-implementation-file indexes.
- Do not keep implementation `.cpp` files at parser top-level once their
  semantic `impl/` directory exists.
- The previous parser facade boundary work already made
  `src/frontend/parser/parser.hpp` a smaller public surface; do not regress it
  by reintroducing private state dependencies.
- Keep the transient `review/step3_parser_facade_boundary_review.md` artifact
  out of lifecycle commits unless the supervisor explicitly requests it.

## Proof

Lifecycle activation only; no build or test proof required.
