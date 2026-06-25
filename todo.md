Status: Active
Source Idea Path: ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: CLI Contract And ELF Text Extraction

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1 from
`ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md`.

## Suggested Next

Execute Step 1 from `plan.md`: implement the initial `c4c-objdump` CLI contract
and supported RV64 relocatable ELF `.text` extraction, with fail-closed
diagnostics and focused backend proof.

## Watchouts

- Do not hard-code the full canonical byte string as disassembly.
- Do not use external objdump/as output as the semantic source of truth.
- Do not touch `ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md`.
- Keep unsupported inputs fail-closed.

## Proof

Lifecycle-only activation. No build or CTest proof required for this slice.
