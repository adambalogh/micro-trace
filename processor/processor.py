import struct
import json
import jsonpickle
from jsonpickle import unpickler
from collections import deque

import db
import trace


def group_spans(spans):
    spans_map = {}
    for s in spans:
        if s.trace() not in spans_map:
            spans_map[s.trace()] = []
        spans_map[s.trace()].append(s)
    return spans_map


def process_trace(spans):
    span_id = {}
    for span in spans:
        span_id[span.span()] = span

    start = None
    for span in spans:
        parent_span = span.parent_span()

        # We found the first span in the trace
        if span.span() == span.trace():
            assert start is None
            start = span
            continue

        # error
        if parent_span not in span_id:
            continue

        span_id[parent_span].add_callee(span)

    return trace.Trace(start)


def process(trace_id_map):
    traces = []
    for trace_id in trace_id_map.keys():
        traces.append(process_trace(trace_id_map[trace_id]))
    traces = [trace for trace in traces if trace.start is not None]
    for trace in traces:
        post_process(trace)
    return traces


def post_process(trace):
    (num_spans, max_end) = traverse(trace.start)
    trace.num_spans = num_spans
    trace.duration = trace.start.duration

def traverse(span):
    num_spans = 1
    max_end = span.time
    for c in span.callees:
        (n, m) = traverse(c)
        num_spans += n
        max_end = max(max_end, m)


    return (num_spans, max_end)


def unprocessed_traces(trace_id_map):
    db_ids = set()
    traces = []
    for trace_id in trace_id_map.keys():
        trace = db.get_trace(trace_id)
        if trace:
            (ids, trace) = extend_trace(trace[1], trace_id_map[trace_id])

            if ids:
                db_ids.update(ids)
                traces.append(trace)
    return (db_ids, traces)


def extend_trace(old_trace_json, spans):
    u = unpickler.Unpickler()
    trace = u.restore(old_trace_json)
    span_map = {}
    q = deque()
    q.append(trace.start)
    while q:
        span = q.pop()
        span_map[span.context.span_id] = span
        for c in span.callees:
            q.append(c)
    ids = set()
    for span in spans:
        if span.context.parent_span in span_map:
            parent_span = span_map[span.context.parent_span]
            parent_span.add_callee(span)
            ids.add(span.db_id)
    return (ids, trace)

if __name__ == "__main__":
    # get spans
    spans = db.load_spans()

    # build traces
    trace_id_map = group_spans(spans)
    traces = process(trace_id_map)

    # delete used spans from db
    db_ids = db.get_db_ids(traces)
    db.delete_spans(db_ids)

    # upload traces to db
    data = json.loads(jsonpickle.encode(traces))
    db.upload(data)

    # try to insert unused spans into existing traces from db
    spans = db.load_spans()
    trace_id_map = group_spans(spans)
    print('got', len(trace_id_map), 'traces unprocessed')
    (ids, new_traces) = unprocessed_traces(trace_id_map)
    print('extended', len(new_traces), 'traces')
    for trace in new_traces:
        post_process(trace)

    db.delete_spans(ids)
    db.delete_traces([trace.start.context.trace_id for trace in new_traces])

    data = json.loads(jsonpickle.encode(new_traces))
    db.upload(data)

