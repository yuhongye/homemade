from flask import Flask, Response
import time
import logging

from opentelemetry.instrumentation.requests import RequestsInstrumentor
from opentelemetry.instrumentation.flask import FlaskInstrumentor

app = Flask(__name__)
FlaskInstrumentor().instrument_app(app)   # ✅ 自动提取 traceparent header
RequestsInstrumentor().instrument()       # ✅ 自动注入 header 到下游请求
logging.basicConfig(level=logging.INFO)

def gs_infer():
    messages = ["Hello", "from", "FastDeploy"]
    for i, msg in enumerate(messages):
        if i == 0:
            logging.info("[FastDeploy] gs_infer - First message: %s", msg)
        yield f"data: {msg}\n\n"
        time.sleep(0.5)
    logging.info("[FastDeploy] gs_infer - Last message: %s", messages[-1])

@app.route("/chat_completions")
def chat_completions():
    return Response(gs_infer(), mimetype="text/event-stream")

if __name__ == "__main__":
    app.run(port=8002, threaded=True)
