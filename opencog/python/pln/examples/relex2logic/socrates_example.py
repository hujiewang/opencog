"""
For running evaluation_to_member_agent.py without the cogserver
"""

from __future__ import print_function
from pln.examples.relex2logic import socrates_agent
from opencog.atomspace import types, AtomSpace, TruthValue
from opencog.scheme_wrapper import load_scm, scheme_eval, scheme_eval_h, __init__
from pln.examples.interactive_agent import InteractiveAgent

__author__ = 'Cosmo Harrigan'

atomspace = AtomSpace()
__init__(atomspace)

coreTypes = "opencog/atomspace/core_types.scm"
utilities = "opencog/scm/utilities.scm"
data = "opencog/python/pln/examples/relex2logic/socrates-r2l.scm"

for item in [coreTypes, utilities, data]:
    load_scm(atomspace, item)

agent = InteractiveAgent(atomspace=atomspace,
                         agent=socrates_agent.SocratesAgent(),
                         num_steps=1000,
                         print_starting_contents=True)
agent.run()
