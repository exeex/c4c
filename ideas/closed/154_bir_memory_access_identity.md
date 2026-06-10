# 154 BIR memory access identity

## Goal

Add BIR-owned memory/access semantic identity for load, store, and address
relationships while keeping target addressing and layout facts outside BIR.

## Why This Exists

Phase A found memory/access source identity to be a BIR-normalization
candidate when limited to semantic operation, source, and stored-value
relationships. Current prepared facts mix that identity with frame layout,
relocation, and target addressing policy.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

## In Scope

- Add BIR memory-access identity attached to load/store/address nodes.
- Store result/stored value names, address space, volatile flag, semantic base
  kind, pointer/global/local/string source identity, same-block global-load
  access, and load-local stored-value source links.
- Use BIR slot/name identity for locals instead of prepared frame slot ids.
- Bridge same-block global-load and load-local stored-value prepared queries.

## Out Of Scope

- Frame slot ids, concrete offsets, size/align layout, base-plus-offset
  legality, TLS register or relocation spelling, GOT/direct/page-low policy,
  target addressing-mode choice, or AArch64 memory operand legality.

## Acceptance Criteria

- BIR memory queries match prepared memory/access identity for local, global,
  pointer, string, volatile, and same-block load/store source cases.
- Store-source and memory-retargeting consumers switch only for semantic
  source identity and leave target addressing downstream.

## Proof Route

Run matching before/after backend memory/codegen coverage plus explicit
equivalence checks for same-block global-load access and load-local
stored-value source recovery. Escalate to broader backend if
address-materialization code is touched.

## Reviewer Reject Signals

- Copies `PreparedAddress` wholesale or embeds frame layout, relocation policy,
  target addressing legality, or AArch64 operand formation into BIR.
- Claims progress by changing memory expectations without preserving target
  addressing behavior.
- Proves only one memory shape while local/global/pointer/string or negative
  paths remain unexamined.

## Closure Notes

Closed after the active runbook completed the narrowed
BIR-representable semantic equivalence scope. The route added BIR-owned memory
identity for direct memory access, same-block global-load identity, and
same-block load-local source identity while preserving prepared lookup helpers
as the oracle/fallback for target-layout, stack-range, relocation, addressing,
and operand-formation facts.

One scalar global-load publication path now consults BIR same-block global-load
identity for representable semantic facts, with the prepared path retained for
legacy or non-BIR-representable positives. Closure does not claim migration of
layout-dependent local-slot/load-local consumers, FP materialization, current
global-load materialization, `globals.cpp`, `alu.cpp`, or `calls.cpp`.

Final accepted proof scope:

`ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'`

Accepted final proof log: `test_before.log`, 4/4 passing.
