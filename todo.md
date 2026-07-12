# Current Packet

Status: Active
Source Idea Path: ideas/open/725_rv64_explicit_register_inline_asm_syntax_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace source-to-BIR constraint syntax

## Just Finished

- Plan Step 1 documented the source-to-BIR syntax flow in `docs/rv64_explicit_register_inline_asm/01_source_to_bir_syntax_flow.md`: source/HIR raw constraint retention, HIR-to-LIR role rendering, LIR storage and shape-only verification, and the closed BIR classifier.
- The trace classifies every admitted token family, identifies `make_inline_asm_metadata` as the first owner/rejection boundary for `{x10}`/`={x10}`, and proves structured clobbers are separate machine-state declarations rather than named-value constraints.

## Suggested Next

- Execute Plan Step 2 by comparing source/LIR/BIR ownership options and deciding whether one narrow RV64 explicit-register operand syntax can be added without broad redesign.

## Watchouts

- Raw physical operand text already survives to LIR; the missing capability is semantic BIR classification/identity, not string transport. The LIR verifier currently checks only result/type shape and `.insn r` operand indices.

## Proof

- No build was required for this documentation-only packet.
- Ran repository citation/path checks for source/HIR lowering, LIR storage/printer/verifier, BIR classification, accepted/unsupported facts, clobber agreement, and focused tests; all cited symbols resolved. `git diff --check` passed.
