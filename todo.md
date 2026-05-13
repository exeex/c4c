Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Existing AArch64 Surface

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized execution
state for Step 1.

## Suggested Next

Delegate Step 1 to an executor: inventory `src/backend/mir/aarch64/`, classify
the existing surface by responsibility, and identify old production `.cpp`
files that must be extracted to markdown in the next packet.

## Watchouts

- Do not debug, patch, or expand old AArch64 `.cpp` files before the markdown
  review and interface ledger are complete.
- Do not add rendered-name recovery or string-based semantic lookup fallback.
- If a required BIR/prepared carrier is missing, split that into a separate
  `ideas/open/` gap idea instead of working around it in target code.

## Proof

Lifecycle-only activation. No build or compile proof required.
