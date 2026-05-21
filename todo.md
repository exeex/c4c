Status: Active
Source Idea Path: ideas/open/356_semantic_bir_pointer_derived_string_loads.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Semantic Dynamic-Load Loss

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/356_semantic_bir_pointer_derived_string_loads.md`.

## Suggested Next

Start Step 1 by reproducing the focused `00173` semantic-BIR evidence and
localizing where a pointer-derived byte load collapses into a fixed
`LoadGlobalInst`.

## Watchouts

- Do not special-case `00173`, one string literal, one global symbol, or one
  loop body.
- Keep direct fixed global/string byte loads distinct from dynamic pointer
  loads.
- Do not reopen AArch64 address-valued memory, recursive `00181` publication,
  runner behavior, expectations, timeout policy, or CTest registration.

## Proof

Lifecycle-only activation. Required check: `git diff --check`.
