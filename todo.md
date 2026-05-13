Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Produce The BIR/Prepared Gap Ledger

# Current Packet

## Just Finished

Step 5: Produce The BIR/Prepared Gap Ledger created
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` as the discoverable
AArch64 BIR / prepared gap ledger against
`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`.

The ledger records present, missing, ambiguous, and deferred statuses for each
required prepared fact and target-local MIR / assembly structure. It identifies
gap ownership across shared preparation, AArch64 MIR design, target ABI work,
assembler/object work, and separate ideas. Its decision is that backend
implementation must not proceed directly to instruction selection or assembly
text emission; Step 6 may proceed to AArch64 prepared-module MIR-boundary idea
selection, while the missing memory volatility/address-space prepared facts
should split a shared-preparation carrier extension first if memory lowering is
selected as the first implementation slice.

## Suggested Next

Next coherent packet: start Step 6 idea-selection work.

Use `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` to choose whether the
next source idea should define the AArch64 prepared-module MIR boundary first,
or split the shared-preparation memory volatility/address-space carrier gap
first if memory lowering is the intended first implementation slice.

## Watchouts

- `BIR_PREPARED_GAP_LEDGER.md` is a gap ledger, not implementation proof; it
  does not establish live AArch64 lowering, assembly, object emission, or
  linking.
- Most required prepared facts are present on `PreparedBirModule`; the
  blocking gap is target-local AArch64 MIR design, with memory
  volatility/address-space preservation as the one visible shared-preparation
  carrier gap.
- Do not fill Step 6 gaps by rendered-name recovery, printed-BIR parsing,
  assembly-string parsing, parser operand recovery, or legacy shape
  recognizers.

## Proof

Commands:

```bash
test -f src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
rg -n "present|missing|ambiguous|deferred|PreparedBirModule|BACKEND_ENTRY_CONTRACT.md|Proceed Versus Split Decision" src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
```

Result: passed. The ledger exists and explicitly records present, missing,
ambiguous, and deferred statuses; references `PreparedBirModule` and
`BACKEND_ENTRY_CONTRACT.md`; and includes a proceed-versus-split decision.
Docs-only packet; no build required and no proof log produced.
