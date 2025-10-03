from langgraph.channels import EphemeralValue, Topic
from langgraph.pregel import Pregel, NodeBuilder

def compute_node1(x):
    print("Start to compute node1", x)
    x = x + x
    print("Finish compute node1", x)
    return x

def compute_node2(x):
    print("Start to compute node2", x)
    y = x["b"] + x["b"]
    print("Finish compute node2", y)
    return y

node1 = (
    NodeBuilder().subscribe_only("a")
    .do(compute_node1)
    .write_to("b", "c")
)

node2 = (
    NodeBuilder().subscribe_to("b")
    .do(compute_node2)
    .write_to("c")
)

channels ={
        "a": EphemeralValue(str),
        "b": EphemeralValue(str),
        "c": Topic(str, accumulate=True),
    }

app = Pregel(
    nodes={"node1": node1, "node2": node2},
    channels = channels,
    input_channels=["a"],
    output_channels=["c"],
)

app.invoke({"a": "foo"})
print(channels["c"])