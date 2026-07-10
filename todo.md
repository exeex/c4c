Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Prepared Contract Evidence

# Current Packet

## Just Finished

Step 1: Refresh Prepared Contract Evidence completed a read-only focused proof
refresh for the six active prepared/CLI rows. Fresh first-owner evidence:

- `backend_prepare_liveness`: first failure is
  `expected helper-built aggregate va_arg ap pointer to publish the concrete ABI argument destination`.
  Owner classification: prepared contract production. The row is asking for a
  concrete call-argument ABI destination publication on the helper-built
  aggregate `va_arg` AP pointer move.
- `backend_prepare_frame_stack_call_contract`: first failure is
  `rv64 FPR ABI/frame fact contract: prepared dump hides FPR facts`. The test
  reaches structured RV64 FPR preserved-value and fixed-frame saved-slot facts
  before checking `prepare::print(prepared)`. Owner classification: printer
  formatting over existing prepared contract facts.
- `backend_prepared_printer`: first failure is
  `[FAIL] missing f128 memory carrier value identity: f128_carrier p.value value_id=0`.
  The refreshed dump contains `f128_carrier p.value value_id=1 kind=memory_backed
  size=16 align=16 bank=fpr class=float width=1 slot_id=#0 stack_offset=0`.
  Owner classification: smaller split, stale value-id exposure/expectation in
  the printer-facing f128 carrier row rather than missing contract production.
- `backend_prealloc_inline_asm`: first failure is the same f128 carrier
  identity mismatch because this CTest row invokes the same
  `backend_prepared_printer_test` executable before reaching inline-asm-specific
  checks. Owner classification: smaller split/stale earlier printer exposure;
  no refreshed evidence yet that prealloc inline-asm publication is the first
  failing owner.
- `backend_cli_dump_prepared_bir_exposes_contract_sections`: first CLI failure
  is `[BACKEND_DUMP_SNIPPET_MISSING] --dump-prepared-bir` for
  `block_entry_publication successor=logic.end.4 status=available to_value_id=9
  to=%t8 home_kind=register destination_kind=value destination_storage=register
  reg=r11`. The CLI output exposes the prepared section headers and contains
  matching `block_entry_publication successor=logic.end.4 ... to=%t8 ... reg=r11`
  rows, but current output prints `to_value_id=10` rather than expected
  `to_value_id=9`. Owner classification: smaller split, stale value-id/exposure
  mismatch unless further producer evidence proves otherwise.
- `backend_cli_dump_prepared_bir_local_arg_call_contract`: first CLI failure is
  `[BACKEND_DUMP_SNIPPET_MISSING] --dump-prepared-bir` for
  `storage %t1 value_id=3 encoding=register bank=gpr placement=gpr:callee_saved#0/w1
  reg=rbx`; refreshed output exposes the storage section but prints
  `storage %t1 value_id=4 encoding=register bank=gpr placement=gpr:callee_saved#0/w1
  reg=rbx width=1 units=rbx`. Owner classification: smaller split, stale
  value-id exposure/expectation in CLI prepared-storage output rather than a
  missing CLI section.

## Suggested Next

Delegate Step 2: Select The First Repair Boundary. The cleanest first
implementation packet is the prepared contract production boundary for
`backend_prepare_liveness` only. Keep the block-entry CLI row, f128 rows, and
local-arg CLI row as stale value-id/exposure splits unless the supervisor
chooses a test-expectation review route or new producer evidence proves a real
contract-production gap.

## Watchouts

- Keep target-specific AArch64 dispatch, RISC-V object emission, RV64 runtime
  lowering, and LLVM torture discovery out of this route unless evidence
  requires supervisor lifecycle routing.
- Do not change expectations, unsupported markers, allowlists, runtime policy,
  timeout policy, baseline accounting, or test classification.
- Do not treat CLI text-only rewrites, helper renames, diagnostic relabeling,
  or classification-only edits as capability progress.
- `backend_prealloc_inline_asm` currently does not prove an inline-asm
  publication owner because the shared executable fails earlier on f128 carrier
  value-id text.
- The local-arg CLI row exposes the requested sections and the same `%t1`
  storage placement, but current value numbering is shifted from expected
  `value_id=3` to actual `value_id=4`; treat that as stale exposure until a
  semantic producer gap is proven.
- The block-entry CLI row likewise exposes the requested
  `logic.end.4`/`%t8`/`r11` publication rows, but current value numbering is
  shifted from expected `to_value_id=9` to actual `to_value_id=10`; do not select
  it as a prepared contract production repair boundary without new evidence.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_inline_asm|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_local_arg_call_contract)$' > test_after.log 2>&1
```

Result: build completed with `ninja: no work to do`; focused CTest failed
6/6 as expected for this evidence-refresh packet. Proof log:
`test_after.log`.
