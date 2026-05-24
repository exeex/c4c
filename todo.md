Status: Active
Source Idea Path: ideas/open/prepared-consumer-missing-fact-diagnostics.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Missing Prepared-Fact Diagnostics

# Current Packet

## Just Finished

Step 1 inventoried missing Prepared-fact diagnostics and chose one narrow
first extraction surface.

Inventory:

- Missing function context:
  - `operands.cpp`: `operand resolution requires a prepared function context`.
    Classification: shared MIR consumer-context-facing. It reports missing
    consumer context, not a missing fact owned by prealloc.
  - `traversal.cpp`: `prepared control-flow function is missing durable
    function identity`. Classification: prealloc/control-flow fact-facing, but
    outside the requested first surface because it is function traversal state.
- Value authority / typed operand authority:
  - `operands.cpp`: `no typed prepared authority exists for value operand`.
    Classification: prealloc fact-owner-facing. It is driven by absence of
    decoded regalloc/storage/value-home authority.
  - `operands.cpp`: `regalloc register assignment is missing typed register
    placement`, `storage-plan register value is missing typed register
    placement`, and `value-home register spelling is diagnostic-only until
    typed placement exists`. Classification: prealloc fact-owner-facing. These
    describe decoded Prepared register facts that lack typed placement.
  - `operands.cpp`: `storage-plan value does not have a supported typed operand
    form` and `prepared value home does not have a supported typed operand
    form`. Classification: prealloc fact-owner-facing. These are unsupported
    decoded Prepared authority categories, not AArch64 emission errors.
- Register conversion:
  - `operands.cpp`: `prepared register placement could not be converted`, or a
    target conversion error message. Classification: target-specific AArch64
    ABI/operand error. Keep local because it uses AArch64 register conversion.
- Storage plan:
  - Operand decoded failures already become `UnsupportedStoragePlan` with a
    storage-plan-specific message. Classification: prealloc fact-owner-facing.
  - Memory/addressing helpers in `dispatch_publication.cpp` silently return
    no match when addressing facts or frame-slot facts are absent.
    Classification: nearby Prepared-consumer fallback behavior, not current
    diagnostic-builder extraction.
- Call plan:
  - `calls.cpp`: `AArch64 call lowering requires an authoritative
    PreparedCallPlan`.
  - `variadic.cpp`: `AArch64 variadic entry helper lowering requires a
    PreparedVariadicEntryPlanFunction`.
    Classification: prealloc fact-owner-facing in principle, but current
    messages are call/variadic lowering-specific and include AArch64 context,
    so keep for a later slice.
- Block mapping and instruction mapping:
  - `dispatch.cpp`: `AArch64 block dispatch requires prepared function and
    block context` and `AArch64 block dispatch could not map prepared block to
    retained BIR instructions`. Classification: shared MIR
    consumer-context-facing or AArch64 dispatch-context-facing, not prealloc
    fact ownership.
  - `dispatch_producers.cpp::find_bir_block(...)` silently returns null for
    missing BIR function, prepared module, block label, or spelling match.
    Classification: block mapping/consumer-context-facing.
- Nearby Prepared-consumer failures:
  - `dispatch_lookup.cpp` value-home lookups and producer helpers return
    null/empty for missing value ids or homes; these are lookup/fallback paths,
    not current diagnostic emitters.
  - `dispatch_publication.cpp` and `dispatch_producers.cpp` use prepared value
    homes, block-entry publication facts, addressing, stack layout, and
    variadic entry facts with local fallback behavior. Classification: mixed
    Prepared fact consumption plus AArch64 emission, not the first diagnostic
    extraction.
  - `prepared_lookups.hpp/.cpp` owns indexed lookup facts for calls,
    address materializations, move bundles, and value homes but currently
    returns null without diagnostic builders.

Chosen first extraction surface: add prealloc diagnostic builders for decoded
prepared home/storage operand authority failures, covering at least these
target-independent categories: missing value authority, missing typed register
authority, unsupported storage-plan authority, and unsupported value-home
authority.

Destination rationale: prealloc owns `PreparedDecodedHomeStorage` status,
source, and kind. The selected messages are facts about missing or unsupported
Prepared authority before target operand construction. Builders can produce a
small target-neutral diagnostic fact/message payload without depending on
AArch64 registers, MIR operands, ABI conversion, or instruction emission.

Target-local categories: missing function/block/instruction consumer context,
block-to-BIR mapping, AArch64 register conversion, ABI/register spelling,
operand legality, instruction-family/terminator support, call/variadic
lowering-specific context, and concrete emission diagnostics remain in
AArch64 or shared MIR as appropriate.

## Suggested Next

Step 2 should add the selected prealloc diagnostic-builder API for decoded
home/storage missing Prepared authority categories, with direct coverage for
missing value authority, missing typed register authority, unsupported storage
plan, and unsupported value home. Do not adapt AArch64 yet.

## Watchouts

Do not weaken messages into generic lowering failures. Do not move AArch64
register conversion, ABI/register spelling, block mapping, instruction mapping,
operand construction, or call/variadic-specific diagnostics into prealloc in
the first extraction.

## Proof

Documentation/inventory-only packet. No implementation files were edited, so
no build or ctest proof was required or run.
