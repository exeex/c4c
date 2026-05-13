Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Define The Backend Entry Contract

# Current Packet

## Just Finished

Step 4: Define The Backend Entry Contract created
`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md` as the discoverable AArch64
backend entry contract.

The contract accepts `PreparedBirModule` as the target-local AArch64 entry
type, rejects raw `bir::Module` as a lowering boundary except as upstream
staging into preparation, records required prepared facts and structured-id
identity rules, defines the target-local MIR/asm facts needed before lowering
resumes, and explicitly rejects rendered-name recovery and string fallback
routes.

## Suggested Next

Next coherent packet: start Step 5 by building the gap ledger against
`BACKEND_ENTRY_CONTRACT.md`.

The Step 5 ledger should compare each required prepared fact and target-local
MIR/asm structure against current BIR / `PreparedBirModule` availability,
record missing or ambiguous facts, and identify whether each gap belongs in
shared preparation, AArch64 MIR design, target ABI work, assembler/object
work, or a separate idea.

## Watchouts

- `BACKEND_ENTRY_CONTRACT.md` is a contract, not implementation proof; it does
  not establish live AArch64 lowering, assembly, object emission, or linking.
- The contract intentionally accepts `PreparedBirModule`; Step 5 should treat
  any needed narrower subset as a future typed prepared subset, not as raw BIR.
- Use `CLASSIFICATION_INDEX.md` only to triage legacy markdown candidates while
  building the gap ledger.
- Do not fill Step 5 gaps by rendered-name recovery, printed-BIR parsing,
  assembly-string parsing, or legacy shape recognizers.

## Proof

Commands:

```bash
test -f src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
rg -n "PreparedBirModule|bir::Module|structured ids|semantic identity|target-local MIR|assembly generation|rendered-name recovery|string fallback|CLASSIFICATION_INDEX.md" src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
```

Result: passed. The contract exists and explicitly mentions/decides
`PreparedBirModule`, raw `bir::Module`, structured ids and semantic identity,
target-local MIR and assembly generation needs, rejection of rendered-name
recovery and string fallback routes, and `CLASSIFICATION_INDEX.md`.
Docs-only packet; no build required and no proof log produced.
