# Current Agent Development Status

Updated 2026-08-16.

## Implemented

- Evidence snapshots are persisted in SQLite, including schema version, source, price basis, time range, portfolio summary, trades, and findings.
- Behavior analysis is deterministic and offline. It reports frequency, symbol concentration, win rate, profit factor, loss asymmetry, and trading during drawdown.
- Finding evidence references are rule-specific: concentration points to the concentrated symbol's buys; loss asymmetry points to sells; drawdown findings point to trades inside the drawdown interval.
- Deterministic review paragraphs include finding codes and evidence trade IDs.
- The read-only `Agent Review` panel loads recent snapshots, renders findings and trades, and highlights referenced trades when a finding is selected.
- A focused Qt test covers snapshot loading, review rendering, finding display, and trade highlighting.
- Yahoo WebSocket support remains experimental. HTTP remains the production quote fallback, and the Agent does not select sources or execute trades.

## Verification

- Qt main build succeeds with `cmd /c .\\build.bat`.
- Qt CTest suite contains six tests, including WebSocket, repository, and Agent Review panel coverage.
- Core CTest remains an independent deterministic test target.
- `git diff --check` is clean apart from pre-existing line-ending warnings where applicable.

## Next Direction

1. Add a provider-neutral `ReviewGenerator` interface while keeping deterministic generation as the offline fallback.
2. Define model-layer timeout, retry, redaction, and cost controls before adding any LLM provider.
3. Persist richer evidence such as drawdown curves and complete historical price sequences.
4. Add Agent Review filtering, pagination, date navigation, and safe snapshot deletion.
5. Keep the Agent read-only and outside the ledger execution path.
