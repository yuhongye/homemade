from flask import Flask, Response
import requests
import logging

from opentelemetry.instrumentation.requests import RequestsInstrumentor
from opentelemetry.instrumentation.flask import FlaskInstrumentor

app = Flask(__name__)
FlaskInstrumentor().instrument_app(app)   # ✅ 自动提取 traceparent header
RequestsInstrumentor().instrument()       # ✅ 自动注入 header 到下游请求
logging.basicConfig(level=logging.INFO)

FASTDEPLOY_URL = "http://localhost:8002/chat_completions"

def stream_from_fastdeploy():
    with requests.get(FASTDEPLOY_URL, stream=True) as r:
        first = True
        for line in r.iter_lines(decode_unicode=True):
            if line.startswith("data:"):
                msg = line[6:]
                if first:
                    logging.info("[RC] First message: %s", msg)
                    first = False
                yield f"{line}\n\n"
        logging.info("[RC] Last message reached")

@app.route("/chat_completions")
def chat_completions():
    return Response(stream_from_fastdeploy(), mimetype="text/event-stream")

if __name__ == "__main__":
    app.run(port=8001, threaded=True)
