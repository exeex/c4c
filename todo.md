Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Owners And Existing Prepared Register Facts

# Current Packet

## Just Finished

Lifecycle activation initialized this packet from `plan.md` Step 1.

## Suggested Next

Inspect the AArch64 ledger, ABI/codegen/module boundaries, prepared register
fact carriers, and local test style. Return the intended owner files and proof
command before implementation begins.

## Watchouts

- Keep `module/` as a prepared/BIR snapshot boundary.
- Do not add instruction selection, assembly emission, object output, or linker
  behavior.
- Do not infer semantics from rendered names or printed BIR.
- Fail closed on unknown or mismatched prepared register facts.

## Proof

Not run; lifecycle activation only.
