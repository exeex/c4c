Status: Active
Source Idea Path: ideas/open/609_rv64_global_data_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RV64 global consumer stops

# Current Packet

## Just Finished

- Activated `ideas/open/609_rv64_global_data_consumer.md` after closing the
  mixed object-data slot authority route.

## Suggested Next

Start Step 1 by inventorying RV64 global consumer stops that now have prepared
authority from `608` and `620`. Build a narrow allowlist that separates
symbol/object emission, relocation-record emission, and access-width rows.

## Watchouts

- Do not produce missing prepared/global authority in this plan.
- Do not infer object bytes, relocation slots, target identity, or access
  widths inside RV64 when prepared facts are absent.
- Keep expectation, unsupported-marker, allowlist, timeout, runtime/link, and
  accounting changes out of the proof.

## Proof

- Not run; activation-only lifecycle packet.
