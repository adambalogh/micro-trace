import struct
import jsonpickle
import json

from pymongo import MongoClient

from trace import *
from gen import request_log_pb2

SIZE_BYTES = 32

def read_spans(file_name):
    spans = []
    with open(file_name, 'rb') as f:
        while True:
            s = f.read(SIZE_BYTES)
            if len(s) != SIZE_BYTES:
                break
            size = int(s, 16)

            proto_bin = f.read(size)
            span = request_log_pb2.RequestLog()
            try:
                ret = span.ParseFromString(proto_bin)
            except:
                break

            # Convert into custom Span obj
            span_obj = make_span(span.time,
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
        if parent_span == span.trace():
            assert start == None
            start = span
            continue

        if parent_span not in span_id:
            print(str(span.trace_id), str(span.span_id), str(span.parent_span))
            continue

        span_id[parent_span].add_callee(span)

    return Trace(start)


def process(trace_id_map):
    traces = []
    for trace_id in trace_id_map.keys():
        traces.append(process_trace(trace_id_map[trace_id]))
    return traces

def traverse(node, margin):
    if node is None:
        return

    print('-' * margin + str(node))
    for c in node.callees:
        traverse(c, margin+2)

def mongo_client():
    client = MongoClient('mongodb://<user>:<password>@ds123193.mlab.com:23193/microtrace')
    return client

# spans = read_spans('log.proto')
# spans.extend(read_spans('log2.proto'))
import test
spans = test.spans

trace_id_map = group_spans(spans)
traces = process(trace_id_map)
data = json.loads(jsonpickle.encode(traces, unpicklable=False))

# client = mongo_client()
# db = client.microtrace
# tc = db.traces_collection

# result = tc.insert_many(data)
# print(result.inserted_ids)

