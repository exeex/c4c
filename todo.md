# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Converge import, core ownership, and Raw publication

## Just Finished

- Plan Step 1 is complete in commit `bf5319467` (`order BIR architecture
  stages and review index`). `src/backend/bir/README.md` now owns the
  normative top-level order, the `S00`-`S29` stage table, all four verifier
  profiles, the `S25 -> S23` allocation retry edge, analysis placement, and
  the complete BIR Markdown review index.
- Fresh inventory/link proof found 43 BIR Markdown files total: the root
  overview plus 42 subordinate files. The root has 42 internal BIR Markdown
  link occurrences resolving to 42 unique targets, with no missing,
  duplicated, or broken target.
- Fresh authority proof found 30 ordered stage rows, four verifier-profile
  rows, one explicit `S25 -> S23` retry edge, root normative-order language,
  sole constraint-interpreter ownership at `S18`, and non-semantic authority
  language for diagnostics.

## Suggested Next

- Execute Plan Step 2 in its declared order: reconcile
  `src/backend/bir/core/README.md`, `src/backend/bir/lir_to_bir/README.md`,
  `src/backend/bir/lir_to_bir/memory/README.md`, and the Raw profile in
  `src/backend/bir/verify/README.md` around one lossless, transactional,
  target-independent Raw publication boundary.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Do not let `pipeline/README.md` supersede the root README's total order.
- Treat existing subordinate stage-order and constraint-authority
  contradictions as ordered inputs to later plan steps, not as permission to
  reopen or broaden completed Step 1.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git show --check --oneline bf5319467` — exit 0; the committed Step 1 slice
  is `order BIR architecture stages and review index`.
- `rg --files src/backend/bir -g '*.md' | sort -u | wc -l` — `43`; excluding
  `src/backend/bir/README.md` — `42`.
- Extracting every `.md` link from `src/backend/bir/README.md`, resolving it
  relative to `src/backend/bir/`, and comparing it with the fresh inventory —
  `42` internal occurrences, `42` unique targets, no missing targets, no
  duplicate targets, and no broken Markdown paths.
- `rg -c '^\\| `S[0-9]{2}` \\|' src/backend/bir/README.md` — `30`.
- `rg -c '^\\| `(Draft/Raw|Canonical|Pseudo|Allocated/MIR-ready)` \\|'
  src/backend/bir/README.md` — `4`.
- `rg -c 'S25 -> S23' src/backend/bir/README.md` — `1`.
- Focused authority search matched the root's normative top-level-order rule,
  `S18` as sole BIR constraint interpreter, diagnostics as never semantic
  authority, and the non-negotiable authority rules.
- `git diff --check --
  ideas/open/731_inline_asm_transport_and_regalloc_contract.md plan.md todo.md`
  — exit 0.
