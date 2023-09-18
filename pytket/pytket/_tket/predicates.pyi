from typing import Any
from typing import Callable, Dict, List

from typing import overload
import pytket._tket.architecture
import pytket._tket.circuit
import pytket._tket.unit_id

class CliffordCircuitPredicate(Predicate):
    def __init__(self) -> None: ...

class CommutableMeasuresPredicate(Predicate):
    def __init__(self) -> None: ...

class CompilationUnit:
    @overload
    def __init__(self, circuit: pytket._tket.circuit.Circuit) -> None: ...
    @overload
    def __init__(self, circuit: pytket._tket.circuit.Circuit, predicates: List[Predicate]) -> None: ...
    def check_all_predicates(self) -> bool: ...
    @property
    def circuit(self) -> pytket._tket.circuit.Circuit: ...
    @property
    def final_map(self) -> Dict[pytket._tket.unit_id.UnitID,pytket._tket.unit_id.UnitID]: ...
    @property
    def initial_map(self) -> Dict[pytket._tket.unit_id.UnitID,pytket._tket.unit_id.UnitID]: ...

class ConnectivityPredicate(Predicate):
    def __init__(self, architecture: pytket._tket.architecture.Architecture) -> None: ...

class DefaultRegisterPredicate(Predicate):
    def __init__(self) -> None: ...

class DirectednessPredicate(Predicate):
    def __init__(self, architecture: pytket._tket.architecture.Architecture) -> None: ...

class GateSetPredicate(Predicate):
    def __init__(self, allowed_types: set[pytket._tket.circuit.OpType]) -> None: ...
    @property
    def gate_set(self) -> set[pytket._tket.circuit.OpType]: ...

class MaxNClRegPredicate(Predicate):
    def __init__(self, arg0: int) -> None: ...

class MaxNQubitsPredicate(Predicate):
    def __init__(self, arg0: int) -> None: ...

class MaxTwoQubitGatesPredicate(Predicate):
    def __init__(self) -> None: ...

class NoBarriersPredicate(Predicate):
    def __init__(self) -> None: ...

class NoClassicalBitsPredicate(Predicate):
    def __init__(self) -> None: ...

class NoClassicalControlPredicate(Predicate):
    def __init__(self) -> None: ...

class NoFastFeedforwardPredicate(Predicate):
    def __init__(self) -> None: ...

class NoMidMeasurePredicate(Predicate):
    def __init__(self) -> None: ...

class NoSymbolsPredicate(Predicate):
    def __init__(self) -> None: ...

class NoWireSwapsPredicate(Predicate):
    def __init__(self) -> None: ...

class NormalisedTK2Predicate(Predicate):
    def __init__(self) -> None: ...

class PlacementPredicate(Predicate):
    @overload
    def __init__(self, architecture: pytket._tket.architecture.Architecture) -> None: ...
    @overload
    def __init__(self, nodes: set[pytket._tket.unit_id.Node]) -> None: ...

class Predicate:
    def __init__(self, *args: Any, **kwargs: Any) -> None: ...
    @classmethod
    def from_dict(cls, arg0: dict) -> Predicate: ...
    def implies(self, other: Predicate) -> bool: ...
    def to_dict(self) -> dict: ...
    def verify(self, circuit: pytket._tket.circuit.Circuit) -> bool: ...

class UserDefinedPredicate(Predicate):
    def __init__(self, check_function: Callable[[pytket._tket.circuit.Circuit],bool]) -> None: ...