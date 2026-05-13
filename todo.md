Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Build The Markdown Classification Index

# Current Packet

## Just Finished

Step 3: Build The Markdown Classification Index created
`src/backend/mir/aarch64/CLASSIFICATION_INDEX.md` as the discoverable AArch64
markdown review index.

The index lists every extracted or inspected AArch64 markdown artifact under
`src/backend/mir/aarch64/`, classifies each artifact, records legacy or
deprecated owner/limitation/removal-condition notes, and calls out the stale
routes that must not influence the new BIR / prepared-backend contract.

## Suggested Next

Next coherent packet: start Step 4 by defining the AArch64 backend entry
contract against current structured BIR / `PreparedBirModule` facts.

The Step 4 contract should decide whether the new route consumes
`PreparedBirModule`, raw `bir::Module`, or a staged subset; specify how
semantic identity is carried through structured ids; identify the target-local
MIR/asm facts needed before lowering resumes; and explicitly reject rendered
name recovery or assembly-string fallback routes.

## Watchouts

- Use `CLASSIFICATION_INDEX.md` as a triage guide, not as proof of current live
  AArch64 capability.
- The index excludes legacy rendered assembly recovery, `ArmCodegen`, parser
  operand recovery, built-in linker orchestration, and old feature-gated
  assembler/linker routes from the Step 4 contract.
- Target-ABI and binary-utils candidates in the index are hypotheses to verify
  against current code, not accepted backend requirements.
- Headers under `src/backend/mir/aarch64/` remain outside this docs-only packet.

## Proof

Commands:

```bash
find src/backend/mir/aarch64 -type f -name '*.md' | sort
bash -lc 'missing=0; for f in $(find src/backend/mir/aarch64 -type f -name "*.md" | sort); do rel=${f#src/backend/mir/aarch64/}; if ! rg -Fq -- "$rel" src/backend/mir/aarch64/CLASSIFICATION_INDEX.md; then echo "missing $rel"; missing=1; fi; done; test -f src/backend/mir/aarch64/CLASSIFICATION_INDEX.md && test "$missing" -eq 0'
```

Result: passed. The first command listed the AArch64 markdown artifacts,
including `CLASSIFICATION_INDEX.md`. The second command confirmed the index
exists and mentions every listed AArch64 markdown artifact. Docs-only packet;
no build required and no proof log produced.
