from trace import *

spans = [

make_span(10, UUID(1, 1), UUID(5, 5), UUID(1, 1)),
make_span(11, UUID(1, 1), UUID(6, 1), UUID(5, 5)),
make_span(15, UUID(1, 1), UUID(9, 9), UUID(6, 1)),
make_span(15, UUID(1, 1), UUID(10, 3), UUID(6, 1)),
make_span(11, UUID(1, 1), UUID(6, 10), UUID(5, 5)),


make_span(10, UUID(2, 1), UUID(5, 5), UUID(2, 1)),
make_span(10, UUID(2, 1), UUID(4, 5), UUID(5, 5)),
make_span(10, UUID(2, 1), UUID(1, 5), UUID(5, 5)),
]
