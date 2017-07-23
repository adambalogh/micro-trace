import struct

from gen import request_log_pb2

SIZE_BYTES = 32

class Trace(object):
    def __init__(self, start):
        self.start = start

class Span(object):
    def __init__(self, trace_id, span_id, parent_span, time):
        self.trace_id = trace_id
        self.span_id = span_id
        self.parent_span = parent_span
        self.time = time
        self.callees = []

    def add_callee(self, callee):
        self.callees.append(callee)

class UUID(object):
    def __init__(self, high, low):
        self.high = high
        self.low = low

    def __hash__(self):
        return hash((self.high, self.low))


def read_spans(file_name):
    spans = []
    with open(file_name, 'rb') as f:
        while True:
            s = f.read(SIZE_BYTES)
            if not s:
                break
            size = int(s, 16)

            proto_bin = f.read(size)
            span = request_log_pb2.RequestLog()
            try:
                ret = span.ParseFromString(proto_bin)
            except:
                break

            # Convert into custom Span obj
            span_obj = Span(span.time,
                    UUID(span.context.trace_id.high,
                        span.context.trace_id.low),
                    UUID(span.context.span_id.high,
                        span.context.span_id.low),
                    UUID(span.context.parent_span.high,
                        span.context.parent_span.low))

            spans.append(span_obj)
            f.read(1) # consume new line
    return spans


def group_spans(spans):
    spans_map = {}
    for s in spans:
        if s.trace_id not in spans_map:
            spans_map[s.trace_id] = []
        spans_map[s.trace_id] = s
    return spans_map

def process_trace(spans):
    start = None
    spans = {}
    for span in spans:
        spans[span_id] = Span(span.time)


def process(trace_id_map):
    traces = []
    for trace_id in trace_id_map.keys():
        traces.append(process_trace(trace_id_map[trace_id]))


spans = read_spans('trace_log.bin')
spans.extend(read_spans('log2.bin'))
trace_id_map = group_spans(spans)

process(trace_id_map)
