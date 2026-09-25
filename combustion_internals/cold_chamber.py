"""Zero-dimensional, non-reacting chamber pressure state."""

from __future__ import annotations

import math


def update_pressure(current_pa: float, inlet_pressures_pa: list[float], dt_s: float, time_constant_s: float) -> float:
    """Relax chamber pressure toward the weakest injector inlet pressure."""
    if not inlet_pressures_pa:
        return current_pa
    if dt_s <= 0.0 or time_constant_s <= 0.0:
        raise ValueError("dt_s and time_constant_s must be positive")
    target_pa = min(inlet_pressures_pa)
    response = 1.0 - math.exp(-dt_s / time_constant_s)
    return current_pa + (target_pa - current_pa) * response