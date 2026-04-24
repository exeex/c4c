Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Header Boundaries

# Current Packet

## Just Finished

- Lifecycle activation only. No implementation packet has run for this plan.

## Suggested Next

- Execute plan Step 1: inventory parser header boundaries and record the first
  code-changing sub-slice.

## Watchouts

- Keep this initiative structural; do not change parser semantics, visible-name
  behavior, AST output, or testcase expectations.
- Idea 93 depends on this parser header hierarchy work and should remain
  inactive until idea 92 is complete or intentionally split.
- `parser.hpp` is currently both public API and private parser implementation
  index; external include sites need to be distinguished from parser `.cpp`
  include sites before moving declarations.

## Proof

- No build/test run for lifecycle-only activation.
