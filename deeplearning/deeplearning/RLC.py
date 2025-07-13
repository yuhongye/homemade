from flask import Flask, Response, request
import requests
import logging

from opentelemetry.instrumentation.requests import RequestsInstrumentor

RequestsInstrumentor().instrument()

app = Flask(__name__)
logging.basicConfig(level=logging.INFO)

RC_URL = "http://localhost:8001/chat_completions"

def stream_from_rc():
    with requests.get(RC_URL, stream=True) as r:
        first = True
        for line in r.iter_lines(decode_unicode=True):
            if line.startswith("data:"):
                msg = line[6:]
                if first:
                    logging.info("[RLC] First message from RC: %s", msg)
                    first = False
                yield f"{line}\n\n"
        logging.info("[RLC] Last message from RC reached")

@app.route("/infer", methods=["POST"])
def infer():
    return Response(stream_from_rc(), mimetype="text/event-stream")

if __name__ == "__main__":
    app.run(port=8000, threaded=True)
