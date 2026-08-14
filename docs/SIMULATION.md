# Simulation core

The simulation module is a Qt-independent C++20 domain layer. UI, network,
and SQLite code must call this layer rather than duplicating trade math.

## Current scope

- Virtual initial cash and current cash
- Buy and sell execution
- Long-only position limits
- Weighted average cost
- Buy fees included in position cost
- Sell fees deducted from proceeds
- Realized and unrealized profit and loss
- Portfolio market value, total equity, and return rate
- Explicit errors for invalid orders and missing valuation prices
- Single-symbol historical buy-and-hold experiments
- Maximum drawdown over the post-purchase equity curve

## Historical experiment contract

`runBuyAndHoldExperiment()` accepts one symbol, initial cash, a start/end
timestamp, a buy fee, and a historical price series. Its rules are:

- Price timestamps must be positive and strictly increasing. Duplicate or
  unsorted timestamps are rejected rather than silently repaired.
- Every price must be finite and greater than zero.
- Execution uses the first price at or after the requested start timestamp.
- Ending valuation uses the last price at or before the requested end
  timestamp. At least one point must exist inside the requested interval.
- Quantity is `floor((initialCash - buyFee) / executionPrice)`. The experiment
  fails when fewer than one share can be purchased.
- The ending position is marked to market and is not automatically sold.
- Maximum drawdown is the largest peak-to-trough loss on total account equity
  from the execution point through the ending point. It is returned as a
  non-negative ratio; `0.20` means 20 percent.

The result records the normalized symbol, actual execution and ending
timestamps/prices, quantity, remaining cash, ending market value/equity, PnL,
return rate, and maximum drawdown. Failures use a typed error plus a readable
message.

## Historical K-line conversion

`buildHistoricalPriceSeries()` converts Qt-independent daily bars into the
strict `PricePoint` input required by the experiment. Callers select either
ordinary close or adjusted close explicitly. The conversion enforces:

- one normalized symbol across the complete series;
- canonical `YYYY-MM-DD` trading dates;
- strictly increasing and unique trading dates;
- finite positive prices under the selected price convention;
- an adjusted close on every bar when adjusted-close mode is selected.

Daily dates become UTC-midnight timestamps as a stable ordering key. This is
not an assertion that a market session occurred at UTC midnight. The Qt bridge
`datahub::toHistoricalPriceSeries()` converts `QVector<KLineData>` into this
domain input. Yahoo K-line parsing retains adjusted close when the response
provides it. Neither layer silently sorts, deduplicates, or drops invalid bars.

## Current boundaries

- Integer share quantities only
- No short selling or leverage
- No taxes, slippage, settlement delay, or trading-calendar checks
- Daily bar dates use ISO calendar dates and UTC midnight only as a canonical
  experiment timestamp; intraday/session timezone conversion is not supported
- The caller must explicitly choose ordinary close or adjusted close
- No split, dividend, delisting, suspension, or other corporate-action handling
- Historical experiments currently support one buy-and-hold symbol and no
  benchmark comparison
- No currency conversion
- In-memory state only; persistence will be added through a separate repository
- Monetary values currently use double; persistence must define rounding rules

## Standalone verification

The core tests do not find or link Qt. Run them from the repository root:

    cmake -S tests/core -B build/core-tests
    cmake --build build/core-tests --config Debug
    ctest --test-dir build/core-tests -C Debug --output-on-failure

The tests cover routing and quote validation plus successful and rejected
trades, weighted cost, partial/full sales, fees, missing prices, realized PnL,
unrealized PnL, total equity, and return rate. Historical experiment tests cover
date-boundary selection, return, drawdown, malformed series, missing interval
data, insufficient funds, and quantity overflow.
