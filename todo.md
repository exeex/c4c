Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement First Real Compile-Time String Folding Surface

# Current Packet

## Just Finished

Plan review consumed `review/consteval_inline_asm_route_review.md` and split
the remaining runbook so Step 4 targets the first real compile-time string
folding surface before any helper integration or closure work.

## Suggested Next

Delegate Step 4. The executor should implement or prove blocked the narrowest
existing compile-time string expression surface that can fold to final inline
asm template text before lowering.

## Watchouts

- Do not treat adjacent string literal token folding as consteval/helper
  support.
- Runtime strings remain intentionally rejected.
- If no existing frontend representation can carry compile-time string-valued
  results without a broader initiative, record the blocker here instead of
  widening the implementation.
- Preserve literal inline asm behavior, operands, constraints, clobbers,
  volatility, memory effects, and `%0` placeholder semantics.

## Proof

No validation was run for this lifecycle-only plan/todo repair.
