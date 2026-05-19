Status: Active
Source Idea Path: ideas/open/308_aarch64_extern_data_symbol_pic_address_formation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Symbol Binding Address Lowering

# Current Packet

## Just Finished

Lifecycle switched from umbrella idea 295 to focused owner idea 308 after the
fresh post-307 backend-regex classification identified `00189.c` as a crisp
AArch64 externally binding data-symbol/PIC relocation residual.

## Suggested Next

Start Step 1 from `plan.md`: trace the `stdout` symbol reference in `00189.c`
to the AArch64 path that emits direct `adrp`/`:lo12:` address formation, then
identify the symbol metadata needed to distinguish external data symbols from
local/internal symbols.

## Watchouts

- Do not implement filename-specific or `stdout`-specific shortcuts.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, runner behavior, timeout policy, or proof logs to claim
  progress.
- Preserve legal direct local/internal symbol address formation.
- Keep runtime, timeout, machine-printer/prepared-node, and `lir_to_bir`
  admission buckets parked under umbrella idea 295.

## Proof

Lifecycle-only switch. No build or tests were run by the plan owner.
