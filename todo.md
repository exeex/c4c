# Current Packet

Status: Active
Source Idea Path: ideas/open/725_rv64_explicit_register_inline_asm_syntax_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace source-to-BIR constraint syntax

## Just Finished

- Applied idea 724's stop condition, parked its incomplete allocation runbook, and activated separate documentation-only idea 725 for the missing RV64 explicit-register operand syntax premise.

## Suggested Next

- Execute Plan Step 1 by tracing accepted and rejected source/LIR/BIR constraint tokens and writing `01_source_to_bir_syntax_flow.md`.

## Watchouts

- Keep clobber syntax separate from named-value operand allocation and do not infer semantic ingress from post-regalloc home validation.

## Proof

- Lifecycle-only switch; no code proof required.
