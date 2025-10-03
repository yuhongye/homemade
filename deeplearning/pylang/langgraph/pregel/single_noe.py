from langgraph.channels import EphemeralValue
from langgraph.pregel import Pregel, NodeBuilder

def compute(x):
    print('Start to compute', x)
    x = x + x
    print('Finish compute', x)
    return x

node1 = (
    NodeBuilder().subscribe_only("a")
    .do(compute)
    .write_to("b")
)

app = Pregel(
    nodes={"node1": node1},
    channels={
        "a": EphemeralValue(str),
        "b": EphemeralValue(str),
    },
    input_channels=["a"],
    output_channels=["b"],
)

app.invoke({"a": "foo"})