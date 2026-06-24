# RV64 Minimal Relocatable ELF Object Emission

## Goal

Add the first direct RV64 relocatable ELF object-emission path for the current
supported backend subset, using the shared native object model instead of
printing assembly and parsing it back.

## Why This Exists

RV64 already has backend machine output and a partial assembler/object writer
port. The next RV64 step is to encode the backend's machine records into
structured object fragments, publish labels and symbols, lower RV64 typed
fixups into ELF relocation records, and produce linkable `.o` files for a
minimal but real subset.

## In Scope

- Add RV64 target fragment or encoder records that carry encoded instruction
  bytes or words plus typed RV64 fixups.
- Integrate RV64 function and data emission with the shared object model.
- Support the minimal relocation set needed for current RV64 backend output,
  including calls, branches or jumps, symbol references, and PC-relative
  address materialization used by supported tests.
- Implement correct RV64 `pcrel_hi` / `pcrel_lo` pairing, including synthetic
  AUIPC-site labels for low relocations when required.
- Emit conservative uncompressed RV64 instructions and linker relaxation hints
  where required; leave RVC compression as a later policy.
- Add object contract and link/runtime smoke tests for RV64 `.o` output beside
  existing asm-route coverage.

## Out Of Scope

- AArch64 object emission.
- CLI default switching or c-testsuite-wide object-route scans.
- GNU-compatible textual assembly parsing.
- RVC compression or linker relaxation optimization beyond correctness hints.
- Replacing the current RV64 `.s` route.
- Broad RV64 backend semantic fixes unrelated to object emission.

## Acceptance And Proof Expectations

- Focused RV64 object tests can inspect `.o` structure with readelf or
  llvm-objdump and link the produced object into a runnable binary for the
  supported smoke subset.
- RV64 object emission uses backend machine records and the shared object
  model, not assembly text round-tripping.
- RV64 relocation tests prove the required `pcrel_hi` / `pcrel_lo` pairing and
  at least one external symbol or call relocation.
- The equivalent asm-route tests still pass.

## Dependency Notes

- Depends on the shared object model/API child:
  `ideas/open/330_native_object_model_and_emission_api.md`.
- Provides target-specific proof required before CLI/test integration can
  expose RV64 `--codegen obj` as a supported route.
- A later c-testsuite scan child should broaden coverage after this child
  establishes narrow linkable RV64 objects.

## Reviewer Reject Signals

- RV64 `.o` emission is implemented by printing `.s` and feeding that text into
  the partial assembler or an external assembler.
- `pcrel_lo` relocations target the final symbol directly when the correct
  relocation requires the AUIPC-site synthetic label.
- Relocation kinds are represented as loose strings or raw numbers throughout
  the RV64 backend instead of target-owned typed fixups.
- The slice passes only a named smoke test through testcase-shaped matching
  while nearby same-feature RV64 calls, labels, or symbol references remain
  unexamined.
- Existing RV64 asm-route tests or expectations are downgraded to hide object
  route gaps.
- The change expands into AArch64, CLI default switching, or c-testsuite-wide
  policy instead of staying on minimal RV64 object emission.
- The object file is structurally present but not linkable for the claimed
  supported subset.
