Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Select The First New Implementation Idea

# Current Packet

## Just Finished

Step 6: Select The First New Implementation Idea created
`ideas/open/204_aarch64_prepared_module_mir_boundary.md` as the selected
follow-up implementation idea.

The selected follow-up defines the first small AArch64 code-oriented slice as a
prepared-module target MIR boundary: a `PreparedBirModule` handoff with an
AArch64 target-profile and AAPCS64 ABI gate, plus structured-id keyed
module/function/block/operand/register/frame/branch/call/move/data side-table
skeletons. It explicitly excludes instruction selection, assembly text
emission, built-in assembler/linker work, rendered-name recovery, and memory
lowering that depends on missing volatility/address-space carriers.

Memory lowering remains blocked until a later route first splits and solves
the shared prepared-carrier gap for memory volatility and non-default address
spaces.

## Suggested Next

Next coherent lifecycle action: supervisor should review the active
markdown-first runbook for close or deactivation. The selected next
implementation source idea is
`ideas/open/204_aarch64_prepared_module_mir_boundary.md`.

Do not begin implementation under this active markdown-first runbook unless the
supervisor explicitly switches lifecycle state.

## Watchouts

- `BIR_PREPARED_GAP_LEDGER.md` is a gap ledger, not implementation proof; it
  does not establish live AArch64 lowering, assembly, object emission, or
  linking.
- The selected follow-up starts with target-local AArch64 MIR boundary design;
  it is not permission to start instruction selection or assembly emission.
- Do not fill Step 6 gaps by rendered-name recovery, printed-BIR parsing,
  assembly-string parsing, parser operand recovery, or legacy shape
  recognizers.
- If memory lowering becomes the first implementation focus later, split the
  shared prepared volatility/address-space carrier first.

## Proof

Commands:

```bash
test -f ideas/open/204_aarch64_prepared_module_mir_boundary.md
rg -n "PreparedBirModule|AAPCS64|instruction selection|assembly text|rendered-name|volatility|address-space|Reviewer Reject Signals" ideas/open/204_aarch64_prepared_module_mir_boundary.md
```

Result: passed. The follow-up idea exists and explicitly records the
`PreparedBirModule` handoff, AAPCS64 gate, instruction-selection and
assembly-text exclusions, rendered-name exclusion, memory volatility /
address-space blocker, and reviewer reject signals. Docs-only packet; no build
required and no proof log produced.
