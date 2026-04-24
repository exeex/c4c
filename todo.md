Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Public Parser Consumers

# Current Packet

## Just Finished

- Ran plan Step 6 closure judgment for idea 92 after supervisor full-suite
  validation.
- Regression guard passed with the supervisor-provided full-suite logs:
  `test_before.log` and `test_after.log` both report 2974/2974 passing, with
  no new failing tests.
- Close rejected: the directory move and bridge removal work is green, but the
  source idea acceptance criteria are not yet satisfied.
- Remaining scope is now narrowed in `plan.md` to the public parser facade and
  private implementation index boundary.

## Suggested Next

- Execute plan Step 1: inventory actual external `Parser` consumers and decide
  the first code-changing slice for making `parser.hpp` a public facade rather
  than the parser implementation map.

## Watchouts

- `parser.hpp` still includes `impl/parser_state.hpp`, so external include
  users still pull private parser state definitions.
- `parser.hpp` still describes itself as the recursive-descent implementation
  navigation index and contains implementation helper declarations.
- `impl/parser_impl.hpp` still says it is a compatibility bridge and includes
  `../parser.hpp`; it is not yet a standalone private implementation index.
- The existing "All members public" project constraint may require a small
  facade/PIMPL-style boundary or an explicit follow-on idea if state hiding is
  too large for idea 92.
- Do not close idea 92 until the source acceptance criteria are met or the
  unresolved facade work is split into a separate open idea.

## Proof

- Supervisor-provided closure validation:
  - `test_before.log`: accepted full-suite baseline from `test_baseline.log`,
    2974/2974 passing.
  - `test_after.log`: current tree after Step 5 bridge removal, 2974/2974
    passing.
  - Regression guard:
    `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  - Result: PASS, passed=2974 failed=0 total=2974 before and after.
