from langgraph.channels import LastValue, EphemeralValue
from langgraph.pregel import Pregel, NodeBuilder

def compute(x):
    print("Start to compute", x)
    x = x + x
    print("Finish compute", x)
    return x

node1 = (
    NodeBuilder().subscribe_only("a")
    .do(compute)
    .write_to("b")
)

node2 = (
    NodeBuilder().subscribe_only("b")
    .do(compute)
    .write_to("c")
)


app = Pregel(
    nodes={"node1": node1, "node2": node2},
    channels={
        "a": EphemeralValue(str),
        "b": LastValue(str),
        "c": EphemeralValue(str),
    },
    input_channels=["a"],
    output_channels=["b", "c"],
)

app.invoke({"a": "foo"})