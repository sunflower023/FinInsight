# Quote fixtures

Offline supplier payloads for parser and Aggregator tests. They contain no
credentials and require no live endpoint.

| File | Expected result |
|---|---|
| yahoo_valid.json | Valid Yahoo quote |
| yahoo_missing_price.json | Invalid quote payload |
| eastmoney_valid.json | Valid six-digit A-share quote |
| eastmoney_null_data.json | Invalid quote payload |
| sina_valid.txt | Valid Sina quote line |
| sina_malformed.txt | Invalid quote payload |

HTTP lifecycle tests should synthesize 429/500/503 responses, timeouts,
cancellation, and context destruction instead of storing those transport
events as supplier payload files.
