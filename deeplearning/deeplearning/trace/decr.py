from opentelemetry import trace
from opentelemetry.trace import TracerProvider, SpanKind
from opentelemetry.sdk.trace import ReadableSpan
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import (
    ConsoleSpanExporter,
    SimpleSpanProcessor,
)
from opentelemetry.sdk.resources import Resource
from opentelemetry.trace import get_current_span
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.propagate import extract, inject

import functools
import requests


# === 配置 OTLP 导出器 ===
otlp_exporter = ConsoleSpanExporter()

# === 配置 SDK Provider ===
resource = Resource.create({
    "service.name": "TW",
    "host.name": "TW-xxxx"
})
provider = TracerProvider(resource=resource)
trace.set_tracer_provider(provider)
span_processor = SimpleSpanProcessor(otlp_exporter)
provider.add_span_processor(span_processor)
tracer = trace.get_tracer(__name__)

def with_span(func):
    @functools.wraps(func)  # 保留原函数签名
    def wrapper(*args, **kwargs):
        with tracer.start_as_current_span("PULL_INFER_DATA", kind=SpanKind.CLIENT) as span:
            span = get_current_span()
            print('span:', span)
            result = func(*args, **kwargs)
            return result
    return wrapper

@with_span
def manually_traced_get(url):
        try:
            headers = {}
            inject(headers)
            print(headers)
            response = requests.get(url)
        except Exception as e:
            raise
        return response

if __name__ == "__main__":
    url = "http://localhost:5001/pull_infer"
    resp = manually_traced_get(url)
    print("Response JSON:", resp)
