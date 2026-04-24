Status: Active
Source Idea Path: ideas/open/87_parser_visible_name_resolution_structured_result.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Visible-Name Resolution Consumers
# Current Packet

## Just Finished

Activation reset for idea 87. No executor packet has landed on this runbook
yet.

## Suggested Next

Inspect visible-name resolution declarations, definitions, and call sites for
Step 1. Classify each consumer as semantic identity, bridge spelling,
diagnostic text, AST spelling, or fallback recovery, then record the smallest
honest first code-changing packet and its proof scope.

## Watchouts

- Keep source idea 87 focused on visible-name result representation, not a
  parser-wide semantic cleanup.
- Do not repair one namespace, alias, value, type, or concept spelling with a
  testcase-shaped shortcut.
- Preserve string-returning helpers as compatibility wrappers until their
  callers have been intentionally retargeted.
- Do not mix this semantic parser identity route with parser/HIR header
  hierarchy work from ideas 92 or 93.

## Proof

Not run; lifecycle activation only.
