##
##

'''
Decodes control protocol for the IKEA BEKANT adjustable-height
sit/stand desk.

This decoder stacks on top of the 'lin' PD. The underlying uart
decoder should be set to 8n1, 19200 bps, lsb-first.
'''

from .pd import Decoder
