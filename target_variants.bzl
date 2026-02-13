targets = [
    # keep sorted
    "shikra",
]

le_variants = [
    # keep sorted
    "perf-defconfig",
    "debug-defconfig",
    "consolidate",
]

def get_all_le_variants():
    return [(t, v) for t in targets for v in le_variants]

def get_all_variants():
    return get_all_le_variants()
