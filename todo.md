Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Public Parser Surface

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/94_parser_public_facade_pimpl_boundary.md`.

## Suggested Next

Execute Step 1 in `plan.md`: inspect `parser.hpp`, the parser private
implementation index, and parser caller/test include sites before changing the
PIMPL boundary.

## Watchouts

- Preserve parser behavior, AST output, diagnostics behavior, and testcase
  expectations.
- Do not expose `impl/parser_state.hpp` through the public facade.
- Keep implementation-only declarations behind
  `src/frontend/parser/impl/parser_impl.hpp` where practical.
- Keep any test-only hooks clearly named and isolated.

## Proof

Lifecycle-only activation; no build proof required.
