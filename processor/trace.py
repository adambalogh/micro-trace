class Trace(object):
    def __init__(self, start):
        self.start = start

        self.duration = None
        self.num_spans = None

"""Uniquely identifies a span"""
class Context(object):
    def __init__(self, trace_id, span_id, parent_span):
        self.trace_id = trace_id
        self.span_id = span_id
        self.parent_span = parent_span

    def __str__(self):
        return '[trace_id: {}, span_id: {}, parent_span: {}]'.format(
                self.trace_id, self.span_id, self.parent_span)

class Span(object):
    def __init__(self, db_id, time, context, client, server, duration):
        self.db_id = db_id
        self.time = time
        self.context = context
        self.client = client
        self.server = server
        self.duration = duration
        self.callees = []

    def add_callee(self, callee):
        self.callees.append(callee)

    def trace(self):
        return self.context.trace_id

    def span(self):
        return self.context.span_id

    def parent_span(self):
        return self.context.parent_span

    def __str__(self):
        return '[time: {}, context: {}, num_callees: {}]'.format(
                str(self.time), str(self.context), str(len(self.callees)))

def make_span(db_id, time, trace_id, span_id, parent_span, client, server, duration):
    return Span(db_id, time, Context(trace_id, span_id, parent_span), client,
            server, duration)

class UUID(object):
    def __init__(self, high, low):
        self.val = str(high) + str(low)

    def __hash__(self):
        return hash(self.val)

    def __getstate__(self):
        return self.val

    def __eq__(self, other):
        return self.val == other.val

    def __ne__(self, other):
        return not(self == other)

    def __str__(self):
        return self.val


