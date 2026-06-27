# RV64 Object Route Global Data And Symbol Memory Runbook

Status: Active
Source Idea: ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md
Activated after: ideas/closed/408_prepared_va_start_destination_address_helper_operand_publication.md

## Purpose

Repair the current RV64 prepared-object route bucket where prepared global
data, global memory facts, or direct global-symbol base-plus-offset addressing
block object emission.

## Goal

Classify and repair reusable RV64 global-data, symbol-memory, and relocation
families so prepared global facts lower to valid RV64 object code without
reconstructing frontend/global-initializer semantics inside target emission.

## Core Rule

RV64 object emission may lower explicit prepared global storage and
global-memory facts. It must not synthesize global initializer bytes, symbol
identity, string identity, or memory-access facts that BIR/prepared output did
not publish.

## Read First

- `ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `ideas/closed/398_rv64_object_route_stack_frame_and_param_home_edges.md`
- `ideas/closed/408_prepared_va_start_destination_address_helper_operand_publication.md`
- `tests/c/external/gcc_torture/src/20010924-1.c`
- `tests/c/external/gcc_torture/src/20001121-1.c`
- `tests/c/external/gcc_torture/src/20031211-1.c`
- `tests/c/external/gcc_torture/src/pr57568.c`
- Current RV64 gcc_torture backend probe artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Global storage representative:
  `tests/c/external/gcc_torture/src/20010924-1.c`
- Prepared global memory access representative:
  `tests/c/external/gcc_torture/src/20001121-1.c`
- Direct global-symbol base-plus-offset representative:
  `tests/c/external/gcc_torture/src/20031211-1.c`
- Additional global/string symbol representative:
  `tests/c/external/gcc_torture/src/pr57568.c`
- Nearby same-bucket global-data cases selected by the supervisor from
  current RV64 gcc_torture backend artifacts.

## Non-Goals

- Do not rewrite frontend constant handling as part of RV64 object emission.
- Do not reconstruct missing global initializer, symbol identity, string data,
  or memory facts in MIR/RV64 object emission.
- Do not reclassify unsupported globals as unsupported tests.
- Do not absorb unrelated instruction, terminator, frame/home, move-bundle, or
  runtime-mismatch work.
- Do not use filename-specific branches, expectation rewrites, or allowlist
  filtering.

## Working Model

The reopened 354 classification found 30 global-data failures:

- 15 `RV64 object route supports only immediate scalar and immediate linear global storage`
- 5 `RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses`
- 5 `RV64 object route requires supported prepared global memory facts`
- 5 `RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing`

The first packet should refresh the representative set and classify which
global-data diagnostics still reproduce in the current tree. Implementation
should only begin after the current global storage, memory-access, relocation,
or producer missing-fact family is named.

## Execution Rules

- Start with classification proof before implementation.
- Keep each implementation packet tied to one concrete prepared global-data or
  symbol-memory family.
- Inspect prepared dumps, object-route diagnostics, and emitted ELF/data
  behavior before editing target emission.
- Preserve symbol identity, relocation kind, data width, signedness, alignment,
  initializer bytes, and qemu behavior.
- If global initializer, symbol-memory, string identity, or base-plus-offset
  facts are missing from prepared output, stop and route that producer
  boundary instead of inferring source/global semantics in RV64 emission.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- If a repaired case object-compiles and links, include qemu comparison in the
  proof.
- Treat malformed ELF/data output, expectation rewrites, allowlist filtering,
  and named-case green proof as route failures.

## Step 1: Classify Current Global Data Rejections

Goal: identify the concrete prepared global storage, global memory, or
direct-symbol addressing facts currently blocking RV64 object emission.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `20010924-1.c`, `20001121-1.c`, `20031211-1.c`, `pr57568.c`, and nearby
  same-bucket global cases.
- Dump or inspect prepared BIR for each representative and record global
  storage facts, initializer shape, symbol identity, memory width, base symbol,
  offset, relocation need, string/global data representation, and object-route
  diagnostic.
- Map each rejection to the RV64 object-route code that emits the current
  global-data diagnostic.
- Decide whether the first repair packet is immediate/linear global storage,
  prepared global memory access width, supported global memory facts,
  direct-symbol base-plus-offset addressing, or producer missing-fact routing.
- Route missing producer facts to lifecycle review instead of patching RV64
  object emission.

Completion check:

- `todo.md` records the concrete global-data family for the first executor
  packet, the representative set, and the exact supervisor-delegated proof
  command.
- Any non-399 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Repair The First Valid Global Data Family

Goal: add reusable RV64 object lowering for the first classified global-data
family when all required prepared facts are explicit.

Actions:

- Update RV64 object emission to consume the prepared global storage,
  global-memory, symbol, offset, or relocation facts for the selected family.
- Preserve correct ELF section contents, symbol references, relocation
  behavior, memory width, alignment, and runtime semantics.
- Preserve existing diagnostics for unsupported or incomplete prepared facts.
- Add or update focused backend coverage where the repo has a matching test
  surface for the global-data family.

Completion check:

- The selected representative no longer fails with the same global-data
  diagnostic.
- Nearby same-family cases examined by the executor either advance together or
  are explicitly classified as separate owners.
- Existing backend tests for adjacent global data, symbol memory, and object
  emission remain green.

## Step 3: Prove Representatives And Residual Ownership

Goal: prove the global-data repair advanced 399 without hiding runtime or
ownership failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for the
  repaired representative and same-family additions.
- If a case object-compiles and links, inspect qemu comparison rather than
  stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same global-data family, a distinct
  target-emission family, a producer-fact gap, or a runtime mismatch that needs
  a separate source idea.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering,
  malformed ELF/data output, reconstructed producer facts, or filename-specific
  fixes are used as acceptance evidence.
- The supervisor has enough evidence to continue with another 399 packet,
  request route review, or ask the plan owner for close/deactivation handling.
