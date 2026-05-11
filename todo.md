Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and Classify Link-Visible Symbol Paths

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and initialized this execution-state file for Step 1.

## Suggested Next

Start Step 1 by inventorying link-visible raw string symbol paths across HIR, LIR, BIR, and backend preparation. Record retained compatibility bridges here before changing implementation files.

## Watchouts

- Do not treat printer output, diagnostics, SSA names, block labels, stack slots, registers, string-pool labels, or inline asm text as `LinkNameId` candidates.
- Do not claim progress through output-only expectation rewrites or unsupported downgrades.
- A valid `LinkNameId` miss for a covered known symbol must not reopen raw string lookup silently.

## Proof

Lifecycle-only activation; no build or test proof required yet.
