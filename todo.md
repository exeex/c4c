Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retarget External Include Sites And Remove Bridges

# Current Packet

## Just Finished

- Completed plan Step 5 by removing the now-unused top-level private
  compatibility bridges `src/frontend/parser/parser_state.hpp` and
  `src/frontend/parser/types_helpers.hpp`.
- Confirmed `rg` include-site checks find no remaining external or parser
  implementation includes of those top-level bridge paths.
- Confirmed external app, HIR, and frontend test callers still include the
  public parser surface through `parser.hpp`; no external include retargeting
  was required.
- Confirmed frontend parser implementation files include the private
  implementation index `impl/parser_impl.hpp`, and type parser implementation
  files include `impl/types/types_helpers.hpp` directly.

## Suggested Next

- Next coherent movement boundary: supervisor should review the completed
  header-hierarchy runbook state and decide whether to route lifecycle closure
  to the plan owner.

## Watchouts

- No bridge had to remain: `parser_state.hpp` and `types_helpers.hpp` only
  appeared as the deleted bridge files themselves before removal.
- `parser.hpp` still includes `impl/parser_state.hpp` because the public
  `Parser` class owns state fields that require those definitions; this packet
  only removed the unused top-level compatibility bridge.
- Backend assembler local `parser.hpp` files were not touched and remain
  unrelated local assembler headers.
- Closure-readiness hazard: broader lifecycle closure still belongs to the
  supervisor and plan owner; this packet only proves the delegated
  `frontend_parser_tests` subset.

## Proof

- Ran exactly:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed.
- Test subset: `frontend_parser_tests`.
- Proof log: `test_after.log`.
