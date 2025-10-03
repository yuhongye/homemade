import logging

from flask import Flask, Response, request
import time
from opentelemetry import trace
from opentelemetry.trace import use_span
from opentelemetry.instrumentation.flask import FlaskInstrumentor
from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.propagate import extract
from opentelemetry.trace import get_current_span
import requests
from opentelemetry.instrumentation.requests import RequestsInstrumentor


# === 配置 OTLP 导出器 ===
otlp_exporter = OTLPSpanExporter(
    endpoint="http://apm-collector.bj.baidubce.com/v1/traces",
    headers={"Authentication": "T58djjKywSYZtIEB6RHP1yig"},
)

# === 配置 SDK Provider ===
resource = Resource.create({
    "service.name": "FastDeploy",
    "host.name": "fastdeploy-0"
})

trace.set_tracer_provider(TracerProvider(resource=resource))
tracer = trace.get_tracer(__name__)
span_processor = BatchSpanProcessor(otlp_exporter)
trace.get_tracer_provider().add_span_processor(span_processor)

# ==== Flask 应用 + Instrument ====
app = Flask(__name__)
FlaskInstrumentor().instrument_app(app)
RequestsInstrumentor().instrument()

logging.basicConfig(level=logging.INFO)

@app.route("/chat_completions")
def chat_completions():
    # ✅ 从请求中提取 trace context
    stream_span = get_current_span()
    print(request.headers)
    def gs_infer():
        messages = ["Hello", "from", "FastDeploy"]
        for i, msg in enumerate(messages):
            if i == 0:
                with use_span(stream_span, end_on_exit=False):
                    with tracer.start_as_current_span("fd_first_token") as span:
                        span.set_attribute("message.token", msg)
                logging.info("[FastDeploy] gs_infer - First message: %s", msg)
            yield f"data: {msg}\n\n"
            time.sleep(0.5)
        logging.info("[FastDeploy] gs_infer - Last message: %s", messages[-1])
        with use_span(stream_span, end_on_exit=False):
            with tracer.start_as_current_span("fd_last_token") as span:
                span.set_attribute("message.token", "[DONE]")
        # stream_span.end()
    return Response(gs_infer(), mimetype="text/event-stream")

if __name__ == "__main__":
    app.run(port=8002, threaded=True)
