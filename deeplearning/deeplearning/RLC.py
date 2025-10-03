import logging

from flask import Flask, Response, request, stream_with_context
import time
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
from opentelemetry.context import attach, detach




# === 配置 OTLP 导出器 ===
otlp_exporter = OTLPSpanExporter(
    endpoint="http://apm-collector.bj.baidubce.com/v1/traces",
    headers={"Authentication": "T58djjKywSYZtIEB6RHP1yig"},
)

# === 配置 SDK Provider ===
resource = Resource.create({
    "service.name": "RLC",
    "host.name": "rlc-0"
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

RC_URL = "http://localhost:8001/chat_completions"



@app.route("/infer", methods=["POST"])
def infer():
    stream_span = get_current_span()
    stream_span.set_attribute("job_id", "job-1234")
    stream_span.set_attribute("data_id", "data-1234")
    stream_span.set_attribute("gen_id", "gen-1234")
    headers = {}
    inject(headers)
    print("traceId:", format(stream_span.get_span_context().trace_id, '032x'))
    print(headers)
    def stream_from_rc():
        with requests.get(RC_URL, headers=headers, stream=True) as r:
            print('inner headers', headers)
            print('inner span', get_current_span())
            first = True
            for line in r.iter_lines(decode_unicode=True):
                if line.startswith("data:"):
                    msg = line[6:]
                    if first:
                        with use_span(stream_span, end_on_exit=False):
                            with tracer.start_as_current_span("rlc_first_token") as span:
                                span.set_attribute("message.token", msg)
                        logging.info("[RLC] First message from RC: %s", msg)
                        first = False
                    yield f"{line}\n\n"
            with use_span(stream_span, end_on_exit=False):
                with tracer.start_as_current_span("rlc_last_token") as span:
                    span.set_attribute("message.token", "[DONE]")
            logging.info("[RLC] Last message from RC reached")
    return Response(stream_with_context(stream_from_rc()), mimetype="text/event-stream")

if __name__ == "__main__":
    app.run(port=8000, threaded=True)
