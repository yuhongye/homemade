from ChatCompletionRequest import ChatCompletionRequest
from FDTraceUtil import inject_to_metadata, extract_from_metadata
from opentelemetry import trace
from opentelemetry.trace import TracerProvider, SpanKind
from opentelemetry.sdk.trace import ReadableSpan
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import (
    ConsoleSpanExporter,
    SimpleSpanProcessor,
    BatchSpanProcessor,
)
from opentelemetry.sdk.resources import Resource
from opentelemetry.trace import get_current_span
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.propagate import extract, inject



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

def entry(request):
    with tracer.start_as_current_span("PULL_INFER_DATA", kind=SpanKind.CLIENT) as span:
        inject_to_metadata(request)
        print(request.metadata)
        print('Finish of entry')

def exit0(request):
    ctx = extract_from_metadata(request)
    print('exit0 context', ctx)
    span = tracer.start_span("/stream", context=ctx)
    print('exit0 span', span)

# request = {"metadata": {'k': 'v'}}
request = ChatCompletionRequest('1234')
entry(request)
print('after entry active span', get_current_span())

exit0(request)


