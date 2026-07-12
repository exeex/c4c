# Current Packet

Status: Active
Source Idea Path: ideas/open/725_rv64_explicit_register_inline_asm_syntax_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide whether narrow syntax support is valid

## Just Finished

- Plan Step 2 compared source/HIR, HIR-to-LIR, LIR/verifier, and semantic BIR ownership in `docs/rv64_explicit_register_inline_asm/02_narrow_support_decision.md` and selected semantic BIR as the sole grammar owner.
- The bounded route accepts canonical RV64 GPR `{xN}`, `={xN}`, and `+{xN}` tokens, publishes BIR-owned bank/physical-index metadata, keeps target legality for prepared admission, specifies precise malformed/unsupported/clobber behavior, and stops before regalloc.

## Suggested Next

- Execute Plan Step 3 by writing the research index and reviewing exact file count, links, consistency, acceptance criteria, and reject signals.

## Watchouts

- Syntax support must remain a separate BIR-only implementation idea. Idea 724 may resume only after a source-backed positive proves structured BIR identity; do not combine syntax with prepared/regalloc enforcement.

## Proof

- No build was required for this documentation-only packet.
- Resolved all cited source/HIR, HIR-to-LIR, LIR/verifier, semantic BIR, target-profile call-site, and test paths/symbols. `git diff --check` passed.
