__version__ = "0.1.4-dev57+7a62b22"

# isort: split

from signal_tl._cext import (Always, And, Const, Eventually, Not, Or,
                             Predicate, Until)
from signal_tl._cext.semantics import compute_robustness
from signal_tl._cext.signal import Sample, Signal, Trace, synchronize

F = Eventually
G = Always
U = Until

TOP = Const(True)
BOT = Const(False)
