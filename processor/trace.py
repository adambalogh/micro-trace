class Trace(object):
    def __init__(self, start):
        self.start = start

class Span(object):
    def __init__(self, time, trace_id, span_id, parent_span):
        self.time = time
        self.trace_id = trace_id
        self.span_id = span_id
        self.parent_span = parent_span
        self.callees = []

    def add_callee(self, callee):
        self.callees.append(callee)

class UUID(object):
    def __init__(self, high, low):
        self.high = high
        self.low = low

    def __hash__(self):
        return hash((self.high, self.low))

    def __eq__(self, other):
        return self.high == other.high and self.low == other.low

    def __ne__(self, other):
        return not(self == other)


