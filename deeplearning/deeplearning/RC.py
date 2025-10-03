import logging

from flask import Flask, Response, request,stream_with_context
import time

from httpx import stream
from opentelemetry import trace
from opentelemetry.trace import use_span
from opentelemetry.instrumentation.flask import FlaskInstrumentor
from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.propagate import extract, inject
from opentelemetry.trace import get_current_span
from opentelemetry.instrumentation.requests import RequestsInstrumentor


import requests


# === 配置 OTLP 导出器 ===
otlp_exporter = OTLPSpanExporter(
    endpoint="http://apm-collector.bj.baidubce.com/v1/traces",
    headers={"Authentication": "T58djjKywSYZtIEB6RHP1yig"},
)

# === 配置 SDK Provider ===
resource = Resource.create({
    "service.name": "RC",
    "host.name": "rc-1"
})

trace.set_tracer_provider(TracerProvider(resource=resource))
tracer = trace.get_tracer(__name__)
span_processor = BatchSpanProcessor(otlp_exporter)
trace.get_tracer_provider().add_span_processor(span_processor)


# ==== Flask 应用 + Instrument ====
app = Flask(__name__)
FlaskInstrumentor().instrument_app(app)  # 自动给每个请求生成 span
RequestsInstrumentor().instrument()
logging.basicConfig(level=logging.INFO)

FASTDEPLOY_URL = "http://localhost:8002/chat_completions"

@app.route("/chat_completions")
def chat_completions():
    print("origin headers", request.headers)
    stream_span = get_current_span()
    headers = {}
    inject(headers)
    print("traceId:", format(stream_span.get_span_context().trace_id, '032x'))
    print(headers)
    def stream_from_fastdeploy():
        with requests.get(FASTDEPLOY_URL, headers=headers, stream=True) as r:
            print('inner span', get_current_span())
            first = True
            for line in r.iter_lines(decode_unicode=True):
                if line.startswith("data:"):
                    msg = line[6:]
                    if first:
                        with use_span(stream_span, end_on_exit=False):
                            with tracer.start_as_current_span("rc_first_token") as span:
                                span.set_attribute("message.token", msg)
                        logging.info("[RC] First message: %s", msg)
                        first = False
                    yield f"{line}\n\n"
            # raise UnicodeError("------")
            logging.info("[RC] Last message reached")
            with use_span(stream_span, end_on_exit=False):
                with tracer.start_as_current_span("rc_last_token") as span:
                    span.set_attribute("message.token", "[DONE]")
            # stream_span.end()
    return Response(stream_with_context(stream_from_fastdeploy()), mimetype="text/event-stream")

if __name__ == "__main__":
    app.run(port=8001, threaded=True)
