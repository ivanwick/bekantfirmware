##
##

## SIGROKDECODE_DIR=~/BEKANT/sigrok/ \
## sigrok-cli \
##   -P 'uart:baudrate=19200:rx=RX,linpyout,BEKANT' \
##   -i ~/BEKANT/logictrace/oem_init_startup.sr

import sigrokdecode
import struct
from collections import namedtuple

FrameRange = namedtuple('FrameRange', ['ss', 'es', 'frame'])

class Ann:
    FRAME = 0
    TRANSACTION = 1
    ERROR = 2

class BusTransaction:
    class Slot:
        def __init__(self, frame_id, short_name, long_name):
            self.frame_id = frame_id
            self.short_name = short_name
            self.long_name = long_name

    bus_sched = [
        Slot(0x11, "Master 0x11", "M 11"),
        Slot(0x08, "Slave 0x08", "S 08"),
        Slot(0x09, "Slave 0x09", "S 09"),
        Slot(0x10, "Slave 0x10", "S 10"),
        Slot(0x10, "Slave 0x10", "S 10"),
        Slot(0x10, "Slave 0x10", "S 10"),
        Slot(0x10, "Slave 0x10", "S 10"),
        Slot(0x10, "Slave 0x10", "S 10"),
        Slot(0x10, "Slave 0x10", "S 10"),
        Slot(0x01, "Slave 0x01", "S 01"),
        Slot(0x12, "Master 0x12", "M 12"),
    ]
    
    def __init__(self):
        self.next_sched_pos = 0
        self.frames = []

    def match_first_frame(self, frame_dict):
        slot = self.bus_sched[0]
        return 'id' in frame_dict and slot.frame_id == frame_dict['id']
    
    def is_init(self):
        return self.next_sched_pos == 0

    def match_next_frame(self, frame_dict):
        if self.next_sched_pos >= len(self.bus_sched):
            return False
        next_slot = self.bus_sched[self.next_sched_pos]
        return 'id' in frame_dict and next_slot.frame_id == frame_dict['id']
            
    def add_frame(self, ss, es, frame_dict):
        self.frames.append(FrameRange(ss=ss, es=es, frame=frame_dict))
        self.next_sched_pos += 1

    def is_complete(self):
        return self.next_sched_pos >= len(self.bus_sched)


class Decoder(sigrokdecode.Decoder):
    api_version = 3
    id = 'BEKANT'
    name = 'BEKANT'
    longname = 'BEKANT Control Protocol'
    desc = 'IKEA BEKANT height-adjustable desk control protocol'
    license = 'gplv2+'
    inputs = ['lin']
    outputs = []
    tags = ['LIN']
    annotations = (
        ('frame', 'LIN frames'),
        ('transaction', 'Transactions'),
        ('error', 'Error descriptions'),
    )
    annotation_rows = (
        ('frame', 'Frame', (Ann.FRAME,)),
        ('transaction', 'Transaction', (Ann.TRANSACTION,)),
        ('error', 'Error', (Ann.ERROR,)),
    )

    def __init__(self):
        self.reset()

    def reset(self):
        self.txn = BusTransaction()

    def start(self):
        self.out_ann = self.register(sigrokdecode.OUTPUT_ANN)

    def putx(self, data):
        # Simplification, most annotations span exactly one SPI byte/packet.
        self.put(self.ss, self.es, self.out_ann, data)

    def putf(self, data):
        self.put(self.ss_field, self.es_field, self.out_ann, data)

    def putc(self, data):
        self.put(self.ss_cmd, self.es_cmd, self.out_ann, data)

    def cmd_ann_list(self):
        s, l = self.current_cmd.short_name, self.current_cmd.long_name
        return ['Command: %s (%s)' % (l, s), 'Command: %s' % l,
                'Cmd: %s' % l, 'Cmd: %s' % s, s]

    command_byte = {
        7: '???',
        0xbc: 'OEM_UP_SLOW?',
        0xbd: 'OEM_DOWN_SLOW?',
        0xbf: 'OEM_bf???',
        0x83: 'OEM_83???',
        0x80: 'OEM_80???',
        0x81: 'OEM_81???',
        0x82: 'OEM_82???',
        0xc0: 'OEM_c0???',
        0xff: 'OEM_ff???',
        0xfc: 'STOP',
        0xc4: 'PRE_MOVE',
        0x86: 'UP',
        0x85: 'DOWN',
        0x87: 'DECEL',
        0x84: 'PRE_STOP',
    }

    def decode(self, ss, es, packet):
        ptype = packet[0]
        pdata = packet[1]

        if ptype == 'FRAME':
            self.decode_frame(ss, es, pdata)
            if self.txn.is_init() and not self.txn.match_first_frame(pdata):
                # not tracking a running transaction yet, ignore frame
                pass
            else:
                if self.txn.match_next_frame(pdata):
                    self.txn.add_frame(ss, es, pdata)
                    if self.txn.is_complete():
                        self.decode_transaction(self.txn)
                        self.reset()
                else:
                    self.put(ss, es, self.out_ann, [Ann.ERROR, ["Error"]])
                    self.reset()
        elif ptype == 'ERROR':
            self.put(ss, es, self.out_ann, [Ann.ERROR, ["Error from LIN decoder"]])

    def decode_transaction(self, transaction):
        frame_08 = transaction.frames[1].frame
        frame_09 = transaction.frames[2].frame
        frame_cmd = transaction.frames[10].frame
        first_ss = transaction.frames[0].ss
        last_es = transaction.frames[10].es
        
        data_08 = data_tuples_as_list(frame_08['data'])
        data_09 = data_tuples_as_list(frame_09['data'])
        data_cmd = data_tuples_as_list(frame_cmd['data'])

        encoder_08 = extract_encoder_val(data_08)
        encoder_09 = extract_encoder_val(data_09)
        encoder_cmd = extract_encoder_val(data_cmd)

        cmd_name = self.command_byte[ data_cmd[2] ]
        self.put(first_ss, last_es, self.out_ann,
            [Ann.TRANSACTION, [
                '08:%04x 09:%04x cmd:%04x %s' % (
                    encoder_08, encoder_09, encoder_cmd, cmd_name),
                '%04x %s' % (encoder_cmd, cmd_name),
                cmd_name
                ]])
        
    def decode_frame(self, ss, es, frame_dict):
        self.put(ss, es, self.out_ann,
            [Ann.FRAME, [
                self.frame_as_string_long(frame_dict),
                # self.frame_as_string_short(f)
                ]])
    

    def frame_as_string_long(self, frame_dict):
        data_list = data_tuples_as_list(frame_dict['data'])
        return "ID:%02x Data:%s" % (
            frame_dict['id'],
            ' '.join(["%02x" % b for b in data_list])
            )

def data_tuples_as_list(dtups):
    return [t[2] for t in dtups]

def extract_encoder_val(data_list):
    (enc,) = struct.unpack("H", bytes(data_list[0:2]))
    return enc
