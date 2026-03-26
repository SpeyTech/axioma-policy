#!/usr/bin/env python3
"""
ax-rtm-verify.py — Requirements Traceability Matrix Verification

DVEC: v1.3
SRS-003 v0.3 — Policy Evaluation & Operational Envelope

Exit codes: 0 = CONFORMANT, 1 = NON-CONFORMANT
"""

import argparse
import re
import sys
from pathlib import Path

SRS_003_REQUIREMENTS = {
    "SRS-003-SHALL-001": "Policy evaluation D2 deterministic",
    "SRS-003-SHALL-002": "Q16.16 fixed-point arithmetic",
    "SRS-003-SHALL-003": "Bit-identical AX:POLICY:v1 records",
    "SRS-003-SHALL-004": "RFC 8785 (JCS) canonicalized",
    "SRS-003-SHALL-005": "Required fields",
    "SRS-003-SHALL-006": "L5 enforcement",
    "SRS-003-SHALL-007": "Violation propagation",
    "SRS-003-SHALL-008": "Traceability",
    "SRS-003-SHALL-009": "Zero allocation",
    "SRS-003-SHALL-010": "Bounded execution",
    "SRS-003-SHALL-011": "Input binding",
    "SRS-003-SHALL-012": "Ordering",
    "SRS-003-SHALL-013": "Policy set closure",
    "SRS-003-SHALL-014": "Aggregation rule",
    "SRS-003-SHALL-015": "Pre-commit invariant",
    "SRS-003-SHALL-016": "Failure semantics",
    "SRS-003-SHALL-017": "Purity constraint",
    "SRS-003-SHALL-018": "Canonical structure",
    "SRS-003-SHALL-019": "Policy identity stability",
    "SRS-003-SHALL-020": "Policy totality",
    "SRS-003-SHALL-021": "One record per policy",
    "SRS-003-SHALL-022": "Cross-layer ordering",
}

def find_anchors(root: Path) -> dict:
    """Find all SRS anchors in source files."""
    anchors = {}
    pattern = re.compile(r'SRS-003-SHALL-\d{3}')
    
    for ext in ['*.c', '*.h']:
        for path in root.rglob(ext):
            if 'build' in str(path):
                continue
            content = path.read_text()
            for match in pattern.findall(content):
                if match not in anchors:
                    anchors[match] = []
                anchors[match].append(str(path.relative_to(root)))
    
    return anchors

def verify_rtm(root: Path) -> bool:
    """Verify requirements traceability matrix."""
    print("=" * 70)
    print("ax-rtm-verify.py — SRS-003 v0.3 Traceability Check")
    print("=" * 70)
    print()
    
    anchors = find_anchors(root)
    
    missing = []
    covered = []
    
    for req_id, desc in SRS_003_REQUIREMENTS.items():
        if req_id in anchors:
            covered.append(req_id)
            files = ", ".join(anchors[req_id][:2])
            print(f"  [COVERED] {req_id}: {files}")
        else:
            missing.append(req_id)
            print(f"  [MISSING] {req_id}: {desc}")
    
    print()
    print("-" * 70)
    print(f"Coverage: {len(covered)}/{len(SRS_003_REQUIREMENTS)} requirements")
    print("-" * 70)
    
    if missing:
        print(f"\nNON-CONFORMANT: {len(missing)} requirements without anchors")
        return False
    else:
        print("\nCONFORMANT: All requirements have code anchors")
        return True

def main():
    parser = argparse.ArgumentParser(description="RTM Verification")
    parser.add_argument("--root", type=Path, default=Path("."),
                        help="Root directory to scan")
    args = parser.parse_args()
    
    if not args.root.exists():
        print(f"Error: {args.root} does not exist")
        sys.exit(1)
    
    success = verify_rtm(args.root)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
