le_targets = [
    "kalama-le",
]

le_variants = [
    # keep sorted
    "debug-defconfig",
    "defconfig",
    "perf-defconfig",
]

def get_all_le_variants():
    return [(t, v) for t in le_targets for v in le_variants]

def get_all_variants():
    return get_all_le_variants()
