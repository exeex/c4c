# AArch64 Atomic Machine Nodes

Status: Open
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

The archived `atomics.md` describes still-valid AArch64 atomic semantics:
ordered loads and stores, fences, read-modify-write operations, compare
exchange, exclusive-access retry loops, and signed narrow-load normalization.
Current compiled AArch64 codegen has no BIR, prepared, selected machine-node,
or printer owner for atomic ordering, atomic RMW, compare exchange, or fence
operations. Frontend/preprocessor compatibility exposes atomic-related syntax
and constants, but those facts do not become structured backend operations.

## Scope

- Define structured atomic operation carriers before AArch64 selection,
  including ordering, width, result mode, pointer, value, and compare-exchange
  failure-ordering facts.
- Lower AArch64 selected machine nodes for atomic loads, stores, fences, RMW
  loops, and compare-exchange loops from those carriers.
- Print AArch64 acquire/release and exclusive-access instruction sequences from
  structured node facts.
- Preserve old-value result semantics for fetch operations and compare-exchange.

## Non-Goals

- Do not rebuild the archived fixed `x0`/`x1`/`x2`/`x3`/`w4` scratch contract.
- Do not infer atomic ordering from text templates or volatile memory flags.
- Do not add named-case atomic shortcuts for a single testcase.
- Do not weaken unsupported atomic tests to claim backend progress.

## Proof Direction

- Ordered atomic load/store cases emit width-correct acquire/release or plain
  AArch64 instructions from structured ordering facts.
- Atomic RMW emits a retrying exclusive-load/store loop and returns the old
  value.
- Compare exchange models boolean and old-value result modes and clears the
  exclusive monitor on compare failure when required.
- Fence lowering emits the expected `dmb` barrier for non-relaxed orderings.
