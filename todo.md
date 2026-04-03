Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

- [ ] Inventory all legacy backend-IR and LLVM rescue paths with file references
- [ ] Expand BIR surface toward parity with emitter-facing `ir.*`
- [ ] Migrate x86/aarch64 emitters to consume BIR directly
- [ ] Delete legacy backend IR files and routing
- [ ] Delete app-layer LLVM asm rescue from `c4cll`
- [ ] Revalidate backend and full-suite behavior without fallback
