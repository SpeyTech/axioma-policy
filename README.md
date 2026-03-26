# axioma-policy

**L4 Policy Evaluation Layer — Certified Operational Envelope**

DVEC: v1.3 | SRS-003 v0.3 | 22 SHALL requirements

## Overview

axioma-policy implements deterministic policy evaluation against the Certified
Operational Envelope (COE). All policy decisions are cryptographically anchored
via `AX:POLICY:v1` evidence records.

Layer Integration:
- L6 (Truth) — what happened
- L5 (Behaviour) — what to do  
- **L4 (Policy) — what is allowed**

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Test

```bash
./build/test_policy_totality
python3 ax-rtm-verify.py --root .
```

## SRS-003 Coverage

| Requirement | Description |
|-------------|-------------|
| SHALL-001 | D2 Determinism |
| SHALL-002 | Q16.16 fixed-point arithmetic |
| SHALL-003 | Bit-identical AX:POLICY:v1 records |
| SHALL-004 | RFC 8785 (JCS) canonicalization |
| SHALL-012 | Evaluation order (ledger_seq ASC, policy_id ASC) |
| SHALL-014 | ANY BREACH → overall BREACH |
| SHALL-020 | Totality (no undefined paths) |
| SHALL-021 | One record per policy |
| SHALL-022 | Cross-layer ordering invariant |

## AX:POLICY:v1 Canonical Format

```json
{"actual":<q16.16>,"ledger_seq":<uint64>,"policy_id":"<string>","result":"<PERMITTED|BREACH>","threshold":<q16.16>}
```

Fields in lexicographic order: actual, ledger_seq, policy_id, result, threshold

## License

Copyright (c) 2026 The Murray Family Innovation Trust
SPDX-License-Identifier: GPL-3.0-or-later
Patent: UK GB2521625.0
