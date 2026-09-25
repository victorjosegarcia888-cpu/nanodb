"""Explicit startup state machine for simulation-only engine studies."""

from __future__ import annotations

from enum import Enum


class StartupState(str, Enum):
    SAFE = "SAFE"
    PURGE = "PURGE"
    SPIN = "SPIN"
    VALVES_CHECK = "VALVES_CHECK"
    COLD_FLOW = "COLD_FLOW"
    IGNITION_AUTHORIZED_SIMULATION = "IGNITION_AUTHORIZED_SIMULATION"
    RUN = "RUN"
    SHUTDOWN = "SHUTDOWN"


TRANSITIONS = {
    StartupState.SAFE: {"start": StartupState.PURGE},
    StartupState.PURGE: {"purge_complete": StartupState.SPIN},
    StartupState.SPIN: {"rpm_ready": StartupState.VALVES_CHECK},
    StartupState.VALVES_CHECK: {"valves_ready": StartupState.COLD_FLOW},
    StartupState.COLD_FLOW: {"cold_flow_stable": StartupState.IGNITION_AUTHORIZED_SIMULATION},
    StartupState.IGNITION_AUTHORIZED_SIMULATION: {"simulation_authorized": StartupState.RUN},
    StartupState.RUN: {"shutdown": StartupState.SHUTDOWN, "fault": StartupState.SHUTDOWN},
    StartupState.SHUTDOWN: {"stopped": StartupState.SAFE},
}


class StartupStateMachine:
    def __init__(self, simulation_only: bool = True) -> None:
        self.simulation_only = simulation_only
        self.state = StartupState.SAFE
        self.history = [self.state]

    def dispatch(self, event: str) -> StartupState:
        if event == "simulation_authorized" and not self.simulation_only:
            raise RuntimeError("only simulation-only authorization is supported")
        next_state = TRANSITIONS.get(self.state, {}).get(event)
        if next_state is None:
            raise ValueError(f"event {event!r} is invalid in state {self.state.value}")
        self.state = next_state
        self.history.append(next_state)
        return self.state